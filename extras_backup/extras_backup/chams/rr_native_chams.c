/* rr_native_chams.c - in-process character material pass for ROB RUN.
 *
 * Injected DLL. Attaches to the IL2CPP runtime, walks the live character
 * views, and drives the real UnityEngine.Material API on the material
 * instances the game renders characters with. Because the writes go through
 * Unity's own setters, the URP batcher sees a dirty material and re-uploads
 * its state; raw external writes to the same memory would be ignored.
 *
 * Applies ZTest=Always and ZWrite=off where the shader exposes those
 * properties, plus an optional tint. Every decision is logged with the
 * shader name and the property probe result so the caller can see which
 * shaders honour the write and which do not.
 *
 * F4: toggle. Original property values are restored when switched off.
 * Log: rr_native_chams.log beside this DLL. Config: rr_native_chams.ini.
 *
 * Field offsets are for build 26615022 (dump.cs): BattleCharacterEntityView
 * CustomizationView 0xB8, BattleCustomizationView 0xC0 (_view 0x20),
 * CharacterCustomizationView material fields 0x68..0xC8.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#define RRNC_MAX_MATERIALS 512
#define RRNC_MAX_VIEWS 128
#define RRNC_MAX_ARRAY 64

/* IL2CPP runtime entry points, resolved from GameAssembly.dll exports. */
typedef void   Il2CppObject;
typedef void   Il2CppClass;
typedef void   Il2CppDomain;
typedef void   Il2CppAssembly;
typedef void   Il2CppImage;
typedef void   Il2CppString;
typedef void   Il2CppType;
typedef void   MethodInfo;
typedef void   Il2CppException;

static Il2CppDomain* (*p_domain_get)(void);
static Il2CppDomain* (*p_thread_attach)(Il2CppDomain*);
static const Il2CppAssembly** (*p_domain_get_assemblies)(const Il2CppDomain*, size_t*);
static Il2CppImage* (*p_assembly_get_image)(const Il2CppAssembly*);
static Il2CppClass* (*p_class_from_name)(const Il2CppImage*, const char*, const char*);
static MethodInfo* (*p_class_get_method_from_name)(Il2CppClass*, const char*, int);
static Il2CppObject* (*p_runtime_invoke)(const MethodInfo*, void*, void**, Il2CppException**);
static Il2CppString* (*p_string_new)(const char*);
static Il2CppClass* (*p_object_get_class)(Il2CppObject*);
static const Il2CppType* (*p_class_get_type)(Il2CppClass*);
static Il2CppObject* (*p_type_get_object)(const Il2CppType*);

static Il2CppDomain* g_domain;
static Il2CppClass* g_material_class;
static Il2CppClass* g_object_class;
static Il2CppClass* g_view_class;
static MethodInfo* g_find_by_type;   /* Object.FindObjectsByType(Type, sortMode) */
static MethodInfo* g_find_of_type;   /* Object.FindObjectsOfType(Type)           */
static MethodInfo* g_set_int, *g_set_float, *g_set_color;
static MethodInfo* g_get_int, *g_get_float, *g_get_color;
static MethodInfo* g_has_property;
static MethodInfo* g_get_shader, *g_get_name;

static volatile LONG g_enabled = 1;
static volatile LONG g_stop;
static HANDLE g_self;
static wchar_t g_log_path[MAX_PATH];

typedef struct {
    Il2CppObject* managed;
    uint64_t native;
    int   have_ztest, have_zwrite, have_color;
    int   orig_ztest;
    float orig_zwrite;
    float orig_color[4];
} RRNC_SLOT;
static RRNC_SLOT g_slots[RRNC_MAX_MATERIALS];
static int g_slot_count;
static int g_overflow_logged;

typedef struct {
    int ztest, zwrite, tint, refresh_ms;
    float color[4];
} RRNC_CONFIG;
static RRNC_CONFIG g_cfg = { 1, 1, 1, 500, { 1.0f, 0.0f, 1.0f, 1.0f } };

static void rrnc_log(const char* fmt, ...) {
    if (!g_log_path[0]) return;
    FILE* f = _wfopen(g_log_path, L"a");
    if (!f) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(f, "[%02u:%02u:%02u.%03u] ", (unsigned)t.wHour, (unsigned)t.wMinute,
            (unsigned)t.wSecond, (unsigned)t.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

/* ---- managed helpers ---------------------------------------------------- */

static Il2CppObject* rrnc_invoke(MethodInfo* method, Il2CppObject* object, void** args) {
    if (!method) return NULL;
    Il2CppException* exception = NULL;
    Il2CppObject* result = p_runtime_invoke(method, object, args, &exception);
    return exception ? NULL : result;
}

static int32_t rrnc_array_length(const Il2CppObject* array) {
    if (!array) return 0;
    return *(const int32_t*)((const char*)array + 0x18);
}

static Il2CppObject* rrnc_array_at(const Il2CppObject* array, int index) {
    if (!array || index < 0 || index >= rrnc_array_length(array)) return NULL;
    const char* data = *(const char* const*)((const char*)array + 0x20);
    if (!data) return NULL;
    return *(Il2CppObject* const*)(data + (size_t)index * sizeof(void*));
}

static void rrnc_utf16_to_ascii(const Il2CppString* string, char* out, size_t capacity) {
    out[0] = 0;
    if (!string || capacity == 0) return;
    const int32_t length = *(const int32_t*)((const char*)string + 0x10);
    const uint16_t* chars = (const uint16_t*)((const char*)string + 0x14);
    size_t written = 0;
    for (int32_t i = 0; i < length && written + 1 < capacity; ++i) {
        uint16_t c = chars[i];
        out[written++] = (c >= 32 && c < 127) ? (char)c : '?';
    }
    out[written] = 0;
}

static Il2CppClass* rrnc_find_class(const char* namespaze, const char* name) {
    size_t count = 0;
    const Il2CppAssembly** assemblies = p_domain_get_assemblies(g_domain, &count);
    if (!assemblies || !count || count > 512) return NULL;
    for (size_t i = 0; i < count; ++i) {
        Il2CppImage* image = p_assembly_get_image(assemblies[i]);
        if (!image) continue;
        Il2CppClass* klass = p_class_from_name(image, namespaze, name);
        if (klass) return klass;
    }
    return NULL;
}

/* ---- material work ------------------------------------------------------ */

static int rrnc_has_property(Il2CppObject* material, const char* name) {
    Il2CppString* key = p_string_new(name);
    void* args[1] = { key };
    Il2CppObject* boxed = rrnc_invoke(g_has_property, material, args);
    if (!boxed) return 0;
    return *(const uint8_t*)((const char*)boxed + 0x10) != 0;
}

static int rrnc_get_int(Il2CppObject* material, const char* name, int* out) {
    Il2CppString* key = p_string_new(name);
    void* args[1] = { key };
    Il2CppObject* boxed = rrnc_invoke(g_get_int, material, args);
    if (!boxed) return 0;
    *out = *(const int32_t*)((const char*)boxed + 0x10);
    return 1;
}

static int rrnc_get_float(Il2CppObject* material, const char* name, float* out) {
    Il2CppString* key = p_string_new(name);
    void* args[1] = { key };
    Il2CppObject* boxed = rrnc_invoke(g_get_float, material, args);
    if (!boxed) return 0;
    *out = *(const float*)((const char*)boxed + 0x10);
    return 1;
}

static int rrnc_get_color(Il2CppObject* material, const char* name, float out[4]) {
    Il2CppString* key = p_string_new(name);
    void* args[1] = { key };
    Il2CppObject* boxed = rrnc_invoke(g_get_color, material, args);
    if (!boxed) return 0;
    memcpy(out, (const char*)boxed + 0x10, 4 * sizeof(float));
    return 1;
}

static void rrnc_set_int(Il2CppObject* material, const char* name, int value) {
    Il2CppString* key = p_string_new(name);
    int32_t raw = value;
    void* args[2] = { key, &raw };
    (void)rrnc_invoke(g_set_int, material, args);
}

static void rrnc_set_float(Il2CppObject* material, const char* name, float value) {
    Il2CppString* key = p_string_new(name);
    void* args[2] = { key, &value };
    (void)rrnc_invoke(g_set_float, material, args);
}

static void rrnc_set_color(Il2CppObject* material, const char* name, const float value[4]) {
    Il2CppString* key = p_string_new(name);
    float copy[4];
    memcpy(copy, value, sizeof(copy));
    void* args[2] = { key, copy };
    (void)rrnc_invoke(g_set_color, material, args);
}

static void rrnc_shader_name(Il2CppObject* material, char* out, size_t capacity) {
    out[0] = 0;
    Il2CppObject* shader = rrnc_invoke(g_get_shader, material, NULL);
    if (!shader) { snprintf(out, capacity, "<no shader>"); return; }
    Il2CppObject* name = rrnc_invoke(g_get_name, shader, NULL);
    rrnc_utf16_to_ascii((const Il2CppString*)name, out, capacity);
    if (!out[0]) snprintf(out, capacity, "<unnamed>");
}

static RRNC_SLOT* rrnc_find_slot(uint64_t native) {
    for (int i = 0; i < g_slot_count; ++i)
        if (g_slots[i].native == native) return &g_slots[i];
    return NULL;
}

static void rrnc_apply_material(Il2CppObject* material) {
    if (!material) return;
    const uint64_t native = *(const uint64_t*)((const char*)material + 0x10);
    if (!native) return;
    if (rrnc_find_slot(native)) return;
    if (g_slot_count >= RRNC_MAX_MATERIALS) {
        if (!g_overflow_logged) { g_overflow_logged = 1; rrnc_log("material table full, ignoring further instances"); }
        return;
    }

    RRNC_SLOT slot;
    memset(&slot, 0, sizeof(slot));
    slot.managed = material;
    slot.native = native;
    const int has_ztest = rrnc_has_property(material, "_ZTest");
    const int has_zwrite = rrnc_has_property(material, "_ZWrite");
    int has_color = rrnc_has_property(material, "_BaseColor");
    const char* color_key = "_BaseColor";
    if (!has_color) {
        has_color = rrnc_has_property(material, "_Color");
        color_key = "_Color";
    }

    if (has_ztest && g_cfg.ztest && rrnc_get_int(material, "_ZTest", &slot.orig_ztest)) {
        rrnc_set_int(material, "_ZTest", 8);            /* CompareFunction.Always */
        slot.have_ztest = 1;
    }
    if (has_zwrite && g_cfg.zwrite && rrnc_get_float(material, "_ZWrite", &slot.orig_zwrite)) {
        rrnc_set_float(material, "_ZWrite", 0.0f);
        slot.have_zwrite = 1;
    }
    if (has_color && g_cfg.tint && rrnc_get_color(material, color_key, slot.orig_color)) {
        rrnc_set_color(material, color_key, g_cfg.color);
        slot.have_color = 1;
    }

    g_slots[g_slot_count++] = slot;

    char shader[96];
    rrnc_shader_name(material, shader, sizeof(shader));
    rrnc_log("material native=%016llX shader=\"%s\" has[ztest=%d zwrite=%d color=%d] applied[z=%d w=%d c=%d]",
             (unsigned long long)native, shader, has_ztest, has_zwrite, has_color,
             slot.have_ztest, slot.have_zwrite, slot.have_color);
}

static void rrnc_apply_array(Il2CppObject* array) {
    if (!array) return;
    int count = rrnc_array_length(array);
    if (count > RRNC_MAX_ARRAY) count = RRNC_MAX_ARRAY;
    for (int i = 0; i < count; ++i) rrnc_apply_material(rrnc_array_at(array, i));
}

static void rrnc_apply_single(Il2CppObject* material) { rrnc_apply_material(material); }

static void rrnc_restore_all(void) {
    for (int i = 0; i < g_slot_count; ++i) {
        RRNC_SLOT* slot = &g_slots[i];
        if (slot->have_ztest) rrnc_set_int(slot->managed, "_ZTest", slot->orig_ztest);
        if (slot->have_zwrite) rrnc_set_float(slot->managed, "_ZWrite", slot->orig_zwrite);
        if (slot->have_color && g_cfg.tint) {
            const char* color_key = rrnc_has_property(slot->managed, "_BaseColor") ? "_BaseColor" : "_Color";
            rrnc_set_color(slot->managed, color_key, slot->orig_color);
        }
    }
    rrnc_log("restored %d material instance(s)", g_slot_count);
    g_slot_count = 0;
}

/* BattleCharacterEntityView.CustomizationView (+0xB8), else
 * BattleCustomizationView (+0xC0) -> _view (+0x20). */
static Il2CppObject* rrnc_customization_view(Il2CppObject* view) {
    Il2CppObject* direct = *(Il2CppObject* const*)((const char*)view + 0xB8);
    if (direct) return direct;
    Il2CppObject* battle = *(Il2CppObject* const*)((const char*)view + 0xC0);
    if (!battle) return NULL;
    return *(Il2CppObject* const*)((const char*)battle + 0x20);
}

static void rrnc_pass(void) {
    if (!g_view_class) return;
    int32_t sort_mode = 0;   /* FindObjectsSortMode.None */
    Il2CppObject* type_object = p_type_get_object(p_class_get_type(g_view_class));
    if (!type_object) return;

    Il2CppObject* views = NULL;
    if (g_find_by_type) {
        void* args[2] = { type_object, &sort_mode };
        views = rrnc_invoke(g_find_by_type, NULL, args);
    }
    if (!views && g_find_of_type) {
        void* args[1] = { type_object };
        views = rrnc_invoke(g_find_of_type, NULL, args);
    }
    if (!views) return;

    int count = rrnc_array_length(views);
    if (count > RRNC_MAX_VIEWS) count = RRNC_MAX_VIEWS;
    for (int i = 0; i < count; ++i) {
        Il2CppObject* view = rrnc_array_at(views, i);
        if (!view) continue;
        Il2CppObject* customization = rrnc_customization_view(view);
        if (!customization) continue;
        rrnc_apply_array(*(Il2CppObject* const*)((const char*)customization + 0x88));  /* body  */
        rrnc_apply_array(*(Il2CppObject* const*)((const char*)customization + 0x90));  /* hair  */
        rrnc_apply_array(*(Il2CppObject* const*)((const char*)customization + 0x80));  /* beard */
        rrnc_apply_array(*(Il2CppObject* const*)((const char*)customization + 0xB8));  /* mask  */
        rrnc_apply_single(*(Il2CppObject* const*)((const char*)customization + 0xA8)); /* head  */
        rrnc_apply_single(*(Il2CppObject* const*)((const char*)customization + 0x98)); /* bald  */
        rrnc_apply_single(*(Il2CppObject* const*)((const char*)customization + 0x78)); /* brows */
        rrnc_apply_single(*(Il2CppObject* const*)((const char*)customization + 0x70)); /* strap */
        rrnc_apply_single(*(Il2CppObject* const*)((const char*)customization + 0x68)); /* bag   */
        rrnc_apply_single(*(Il2CppObject* const*)((const char*)customization + 0xA0)); /* bristle */
    }
}

/* ---- config ------------------------------------------------------------- */

static int rrnc_parse_hex_color(const char* text, float out[4]) {
    unsigned long value = strtoul(text, NULL, 16);
    int digits = 0;
    for (const char* p = text; *p && *p != ' ' && *p != '\r' && *p != '\n'; ++p) ++digits;
    if (digits <= 6) {
        out[0] = (float)((value >> 16) & 0xFF) / 255.0f;
        out[1] = (float)((value >> 8) & 0xFF) / 255.0f;
        out[2] = (float)(value & 0xFF) / 255.0f;
        out[3] = 1.0f;
    } else {
        out[0] = (float)((value >> 24) & 0xFF) / 255.0f;
        out[1] = (float)((value >> 16) & 0xFF) / 255.0f;
        out[2] = (float)((value >> 8) & 0xFF) / 255.0f;
        out[3] = (float)(value & 0xFF) / 255.0f;
    }
    return 1;
}

static void rrnc_load_config(void) {
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(g_self, path, MAX_PATH)) return;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return;
    wcscpy_s(slash + 1, (size_t)(path + MAX_PATH - (slash + 1)), L"rr_native_chams.ini");
    FILE* f = _wfopen(path, L"r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char* key = line;
        char* value = eq + 1;
        while (*key == ' ' || *key == '\t') ++key;
        while (*value == ' ' || *value == '\t') ++value;
        for (char* p = key + strlen(key); p > key && (p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\r' || p[-1] == '\n'); --p) *--p = 0;
        if (!strcmp(key, "ztest")) g_cfg.ztest = atoi(value) != 0;
        else if (!strcmp(key, "zwrite")) g_cfg.zwrite = atoi(value) != 0;
        else if (!strcmp(key, "tint")) g_cfg.tint = atoi(value) != 0;
        else if (!strcmp(key, "color")) rrnc_parse_hex_color(value, g_cfg.color);
        else if (!strcmp(key, "refresh_ms")) {
            int ms = atoi(value);
            if (ms >= 100 && ms <= 5000) g_cfg.refresh_ms = ms;
        }
    }
    fclose(f);
}

/* ---- worker ------------------------------------------------------------- */

static int rrnc_resolve(void) {
    HMODULE game_assembly = GetModuleHandleA("GameAssembly.dll");
    if (!game_assembly) return 0;
#define RRNC_RESOLVE(name, var) do { \
        void* proc = (void*)GetProcAddress(game_assembly, name); \
        if (!proc) return 0; \
        memcpy(&(var), &proc, sizeof(proc)); \
    } while (0)
    RRNC_RESOLVE("il2cpp_domain_get", p_domain_get);
    RRNC_RESOLVE("il2cpp_thread_attach", p_thread_attach);
    RRNC_RESOLVE("il2cpp_domain_get_assemblies", p_domain_get_assemblies);
    RRNC_RESOLVE("il2cpp_assembly_get_image", p_assembly_get_image);
    RRNC_RESOLVE("il2cpp_class_from_name", p_class_from_name);
    RRNC_RESOLVE("il2cpp_class_get_method_from_name", p_class_get_method_from_name);
    RRNC_RESOLVE("il2cpp_runtime_invoke", p_runtime_invoke);
    RRNC_RESOLVE("il2cpp_string_new", p_string_new);
    RRNC_RESOLVE("il2cpp_object_get_class", p_object_get_class);
    RRNC_RESOLVE("il2cpp_class_get_type", p_class_get_type);
    RRNC_RESOLVE("il2cpp_type_get_object", p_type_get_object);
#undef RRNC_RESOLVE
    return 1;
}

static int rrnc_bind(void) {
    g_material_class = rrnc_find_class("UnityEngine", "Material");
    g_object_class = rrnc_find_class("UnityEngine", "Object");
    g_view_class = rrnc_find_class("Game.Battle.EntityModel.BattleCharacter.Views", "BattleCharacterEntityView");
    if (!g_material_class || !g_object_class || !g_view_class) return 0;

    g_set_int = p_class_get_method_from_name(g_material_class, "SetInt", 2);
    g_set_float = p_class_get_method_from_name(g_material_class, "SetFloat", 2);
    g_set_color = p_class_get_method_from_name(g_material_class, "SetColor", 2);
    g_get_int = p_class_get_method_from_name(g_material_class, "GetInt", 1);
    g_get_float = p_class_get_method_from_name(g_material_class, "GetFloat", 1);
    g_get_color = p_class_get_method_from_name(g_material_class, "GetColor", 1);
    g_has_property = p_class_get_method_from_name(g_material_class, "HasProperty", 1);
    g_get_shader = p_class_get_method_from_name(g_material_class, "get_shader", 0);
    g_get_name = p_class_get_method_from_name(g_object_class, "get_name", 0);
    if (!g_set_int || !g_set_float || !g_set_color || !g_has_property || !g_get_shader || !g_get_name)
        return 0;

    g_find_by_type = p_class_get_method_from_name(g_object_class, "FindObjectsByType", 2);
    g_find_of_type = p_class_get_method_from_name(g_object_class, "FindObjectsOfType", 1);
    return g_find_by_type || g_find_of_type;
}

static DWORD WINAPI rrnc_worker(LPVOID unused) {
    (void)unused;
    rrnc_load_config();
    rrnc_log("rr_native_chams attached (ztest=%d zwrite=%d tint=%d refresh=%dms)",
             g_cfg.ztest, g_cfg.zwrite, g_cfg.tint, g_cfg.refresh_ms);
    if (!rrnc_resolve()) {
        rrnc_log("GameAssembly.dll exports unavailable - nothing to do");
        return 0;
    }
    g_domain = p_domain_get();
    if (!g_domain || !p_thread_attach(g_domain)) {
        rrnc_log("could not attach to the IL2CPP domain");
        return 0;
    }
    if (!rrnc_bind()) {
        rrnc_log("class/method lookup failed - build layout mismatch");
        return 0;
    }
    rrnc_log("il2cpp attached; find=%s", g_find_by_type ? "FindObjectsByType" : "FindObjectsOfType");

    int key_down = 0;
    while (!InterlockedCompareExchange(&g_stop, 0, 0)) {
        const int down = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
        if (down && !key_down) {
            InterlockedExchange(&g_enabled, !InterlockedCompareExchange(&g_enabled, 0, 0));
            rrnc_log("toggle %s", InterlockedCompareExchange(&g_enabled, 0, 0) ? "on" : "off");
            if (!InterlockedCompareExchange(&g_enabled, 0, 0)) rrnc_restore_all();
        }
        key_down = down;
        if (InterlockedCompareExchange(&g_enabled, 0, 0)) rrnc_pass();
        Sleep((DWORD)g_cfg.refresh_ms);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = module;
        DisableThreadLibraryCalls(module);
        if (GetModuleFileNameW(module, g_log_path, MAX_PATH)) {
            wchar_t* slash = wcsrchr(g_log_path, L'\\');
            if (slash) wcscpy_s(slash + 1, (size_t)(g_log_path + MAX_PATH - (slash + 1)),
                                L"rr_native_chams.log");
        }
        HANDLE thread = CreateThread(NULL, 0, rrnc_worker, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
