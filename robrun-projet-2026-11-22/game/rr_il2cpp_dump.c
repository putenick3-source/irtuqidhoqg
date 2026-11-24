/* rr_il2cpp_dump.c - v2. Emits %LOCALAPPDATA%\RobRunInternal\il2cpp_statics.log
 * Type-driven static-field storage RVAs (owner class irrelevant) plus
 * CharacterReplicatedTransformModel::SetRotation RVA. Live runtime only. */
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

static const char* g_targets[] = {
    "WorldModel", "WorldEntitiesModel", "BattleCameraModel",
    "BattleCharacterModel", "BattleModel", NULL
};

static FILE* g_log;
static unsigned char* g_base;
static volatile LONG g_started;

static void LOG(const char* fmt, ...)
{
    va_list a;
    if (!g_log) return;
    va_start(a, fmt); vfprintf(g_log, fmt, a); va_end(a); fflush(g_log);
}

typedef void* (*p_domain_get)(void);
typedef void* (*p_domain_assembly_open)(void*, const char*);
typedef void* (*p_assembly_get_image)(void*);
typedef const void** (*p_domain_get_assemblies)(void*, size_t*);
typedef size_t (*p_image_get_class_count)(void*);
typedef void* (*p_image_get_class)(void*, size_t);
typedef const char* (*p_class_get_name)(void*);
typedef const char* (*p_class_get_namespace)(void*);
typedef void* (*p_class_get_fields)(void*, void**);
typedef void* (*p_class_get_methods)(void*, void**);
typedef void* (*p_class_get_static_field_data)(void*);
typedef unsigned int (*p_field_get_flags)(void*);
typedef size_t (*p_field_get_offset)(void*);
typedef void* (*p_field_get_type)(void*);
typedef const char* (*p_type_get_name)(void*);
typedef const char* (*p_field_get_name)(void*);
typedef const char* (*p_method_get_name)(void*);
typedef void* (*p_method_get_pointer)(void*);

static p_domain_get f_domain_get;
static p_domain_assembly_open f_domain_assembly_open;
static p_assembly_get_image f_assembly_get_image;
static p_domain_get_assemblies f_domain_get_assemblies;
static p_image_get_class_count f_image_get_class_count;
static p_image_get_class f_image_get_class;
static p_class_get_name f_class_get_name;
static p_class_get_namespace f_class_get_namespace;
static p_class_get_fields f_class_get_fields;
static p_class_get_methods f_class_get_methods;
static p_class_get_static_field_data f_class_get_static_field_data;
static p_field_get_flags f_field_get_flags;
static p_field_get_offset f_field_get_offset;
static p_field_get_type f_field_get_type;
static p_type_get_name f_type_get_name;
static p_field_get_name f_field_get_name;
static p_method_get_name f_method_get_name;
static p_method_get_pointer f_method_get_pointer;

#define RES(v, n) do { *(FARPROC*)&v = GetProcAddress(g_ga, n); \
    if (!v) LOG("MISSING EXPORT %s\n", n); } while (0)

static int resolve(HMODULE g_ga)
{
    RES(f_domain_get, "il2cpp_domain_get");
    RES(f_domain_assembly_open, "il2cpp_domain_assembly_open");
    RES(f_assembly_get_image, "il2cpp_assembly_get_image");
    RES(f_domain_get_assemblies, "il2cpp_domain_get_assemblies");
    RES(f_image_get_class_count, "il2cpp_image_get_class_count");
    RES(f_image_get_class, "il2cpp_image_get_class");
    RES(f_class_get_name, "il2cpp_class_get_name");
    RES(f_class_get_namespace, "il2cpp_class_get_namespace");
    RES(f_class_get_fields, "il2cpp_class_get_fields");
    RES(f_class_get_methods, "il2cpp_class_get_methods");
    RES(f_class_get_static_field_data, "il2cpp_class_get_static_field_data");
    RES(f_field_get_flags, "il2cpp_field_get_flags");
    RES(f_field_get_offset, "il2cpp_field_get_offset");
    RES(f_field_get_type, "il2cpp_field_get_type");
    RES(f_type_get_name, "il2cpp_type_get_name");
    RES(f_field_get_name, "il2cpp_field_get_name");
    RES(f_method_get_name, "il2cpp_method_get_name");
    RES(f_method_get_pointer, "il2cpp_method_get_pointer");
    return 0;
}

static int do_dump(HMODULE g_ga)
{
    void* domain = NULL;
    const void** assemblies;
    size_t acount = 0, ai, i;
    int hits = 0;

    if (resolve(g_ga)) return 1;
    LOG("exports ok base=%p\n", g_base);

    if (!f_domain_get || !f_domain_get_assemblies || !f_assembly_get_image ||
        !f_image_get_class_count || !f_image_get_class || !f_class_get_name ||
        !f_class_get_namespace || !f_class_get_fields ||
        !f_class_get_static_field_data || !f_field_get_flags ||
        !f_field_get_offset || !f_field_get_type || !f_type_get_name ||
        !f_field_get_name) { LOG("critical export missing, abort\n"); return 1; }

    for (i = 0; i < 600; i++) { domain = f_domain_get(); if (domain) break; Sleep(100); }
    if (!domain) { LOG("domain not initialised after 60s\n"); return 1; }
    LOG("domain=%p\n", domain);

    assemblies = f_domain_get_assemblies(domain, &acount);
    if (!assemblies) { LOG("domain_get_assemblies failed\n"); return 1; }
    LOG("assemblies=%zu\n", acount);

    for (ai = 0; ai < acount; ai++) {
        void* image = f_assembly_get_image((void*)assemblies[ai]);
        size_t count;

        if (!image) continue;
        count = f_image_get_class_count(image);

        for (i = 0; i < count; i++) {
            void* klass = f_image_get_class(image, i);
            const char* cname;
            const char* nspace;
            void* sdata;
            void* it;
            void* fld;

            if ((i % 20000) == 0) LOG("scan %zu/%zu hits=%d\n", i, count, hits);
            if (!klass) continue;
            cname = f_class_get_name(klass);
            if (!cname) continue;
            nspace = f_class_get_namespace(klass);

            if (f_method_get_pointer && f_method_get_name &&
                strcmp(cname, "CharacterReplicatedTransformModel") == 0) {
                void* mit = NULL;
                void* m;
                while ((m = f_class_get_methods(klass, &mit)) != NULL) {
                    const char* mn = f_method_get_name(m);
                    void* mp;
                    if (!mn || strcmp(mn, "SetRotation") != 0) continue;
                    mp = f_method_get_pointer(m);
                    if (!mp) continue;
                    LOG("METHOD %s.%s::%s RVA 0x%llX\n", nspace ? nspace : "",
                        cname, mn, (unsigned long long)((unsigned char*)mp - g_base));
                    hits++;
                }
            }

            sdata = f_class_get_static_field_data(klass);
            if (!sdata) continue;

            it = NULL;
            while ((fld = f_class_get_fields(klass, &it)) != NULL) {
                const char* fname;
                void* ftype;
                const char* tname;
                unsigned char* addr;
                int t;

                if (!(f_field_get_flags(fld) & 0x0010u)) continue;
                fname = f_field_get_name(fld);
                ftype = f_field_get_type(fld);
                if (!fname || !ftype) continue;
                tname = f_type_get_name(ftype);
                if (!tname) continue;

                for (t = 0; g_targets[t]; t++) {
                    if (!strstr(tname, g_targets[t])) continue;
                    addr = (unsigned char*)sdata + f_field_get_offset(fld);
                    LOG("STATIC %s.%s::%s type=%s off=0x%zX RVA 0x%llX value=%p\n",
                        nspace ? nspace : "", cname, fname, tname,
                        f_field_get_offset(fld),
                        (unsigned long long)(addr - g_base), *(void**)addr);
                    hits++;
                    break;
                }
            }
        }
        LOG("asm %zu/%zu hits=%d\n", ai + 1, acount, hits);
    }

    LOG("hits=%d\n", hits);
    return hits ? 0 : 2;
}
static int open_log(void)
{
    wchar_t dir[MAX_PATH];
    wchar_t path[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
    if (!n || n >= MAX_PATH) return 1;
    wcscpy_s(path, MAX_PATH, dir);
    wcscat_s(path, MAX_PATH, L"\\RobRunInternal");
    CreateDirectoryW(path, NULL);
    wcscat_s(path, MAX_PATH, L"\\il2cpp_statics.log");
    _wfopen_s(&g_log, path, L"w");
    return g_log ? 0 : 1;
}

void rr_il2cpp_dump(void)
{
    HMODULE ga;
    if (InterlockedExchange(&g_started, 1)) return;
    ga = GetModuleHandleW(L"GameAssembly.dll");
    if (!ga) return;
    g_base = (unsigned char*)ga;
    if (open_log()) return;
    LOG("rr_il2cpp_dump v3 ga=%p\n", ga);
    do_dump(ga);
    LOG("done\n");
}

void rr_il2cpp_dump_run(void) { rr_il2cpp_dump(); }
void rr_dump_il2cpp(void) { rr_il2cpp_dump(); }


