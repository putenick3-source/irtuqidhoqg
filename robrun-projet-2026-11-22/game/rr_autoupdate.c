/* rr_autoupdate.c - live offset resolver. Matches classes and members by name
 * against the running image and emits a full generated_offsets.h. Field
 * offsets are exact; method RVAs come from the MethodInfo head and must land
 * inside the image. Short class names try every homonym until one carries the
 * member. Unconfirmed entries keep the compiled value and log their context. */
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct {
    const char* macro;
    const char* cls;
    const char* member;
    unsigned char kind;
    unsigned int cur;
} rr_au_row;

#include "game/rr_autoupdate_table.h"

#define AU_NAME  192
#define AU_SHORT 96
#define AU_MAX   400000

typedef struct {
    void* klass;
    char full[AU_NAME];
    char shortname[AU_SHORT];
} au_class;

static FILE* g_log;
static unsigned char* g_base;
static size_t g_size;
static au_class* g_all;
static size_t g_n;
static size_t* g_by_full;
static size_t* g_by_short;
static volatile LONG g_started;

static void LOG(const char* fmt, ...)
{
    va_list a;
    if (!g_log) return;
    va_start(a, fmt); vfprintf(g_log, fmt, a); va_end(a); fflush(g_log);
}

typedef void* (*p_domain_get)(void);
typedef const void** (*p_domain_get_assemblies)(void*, size_t*);
typedef void* (*p_assembly_get_image)(void*);
typedef size_t (*p_image_get_class_count)(void*);
typedef void* (*p_image_get_class)(void*, size_t);
typedef const char* (*p_class_get_name)(void*);
typedef const char* (*p_class_get_namespace)(void*);
typedef void* (*p_class_get_fields)(void*, void**);
typedef void* (*p_class_get_methods)(void*, void**);
typedef unsigned int (*p_field_get_flags)(void*);
typedef size_t (*p_field_get_offset)(void*);
typedef const char* (*p_field_get_name)(void*);
typedef void* (*p_field_get_type)(void*);
typedef const char* (*p_type_get_name)(void*);
typedef const char* (*p_method_get_name)(void*);

static p_domain_get f_domain_get;
static p_domain_get_assemblies f_domain_get_assemblies;
static p_assembly_get_image f_assembly_get_image;
static p_image_get_class_count f_image_get_class_count;
static p_image_get_class f_image_get_class;
static p_class_get_name f_class_get_name;
static p_class_get_namespace f_class_get_namespace;
static p_class_get_fields f_class_get_fields;
static p_class_get_methods f_class_get_methods;
static p_field_get_flags f_field_get_flags;
static p_field_get_offset f_field_get_offset;
static p_field_get_name f_field_get_name;
static p_field_get_type f_field_get_type;
static p_type_get_name f_type_get_name;
static p_method_get_name f_method_get_name;

static int image_range(void)
{
    HMODULE m = GetModuleHandleW(L"GameAssembly.dll");
    IMAGE_DOS_HEADER* dos;
    IMAGE_NT_HEADERS64* nt;

    if (!m) return 1;
    g_base = (unsigned char*)m;
    dos = (IMAGE_DOS_HEADER*)m;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 1;
    nt = (IMAGE_NT_HEADERS64*)(g_base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 1;
    g_size = nt->OptionalHeader.SizeOfImage;
    return g_size ? 0 : 1;
}

static int cmp_full(const void* a, const void* b)
{
    return strcmp(g_all[*(const size_t*)a].full, g_all[*(const size_t*)b].full);
}

static int cmp_short(const void* a, const void* b)
{
    return strcmp(g_all[*(const size_t*)a].shortname, g_all[*(const size_t*)b].shortname);
}

static void* find_in(const char* key, size_t* idx, int by_full)
{
    size_t lo = 0, hi = g_n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        size_t k = idx[mid];
        int c = by_full ? strcmp(g_all[k].full, key) : strcmp(g_all[k].shortname, key);
        if (c == 0) return g_all[k].klass;
        if (c < 0) lo = mid + 1; else hi = mid;
    }
    return NULL;
}

static void* resolve_class(const char* cls)
{
    if (!cls || !*cls) return NULL;
    if (strchr(cls, '.')) return find_in(cls, g_by_full, 1);
    return find_in(cls, g_by_short, 0);
}

static size_t lower_bound_short(const char* key)
{
    size_t lo = 0, hi = g_n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (strcmp(g_all[g_by_short[mid]].shortname, key) < 0) lo = mid + 1; else hi = mid;
    }
    return lo;
}

static void log_fields(void* klass, const char* macro)
{
    void* it = NULL;
    void* fld;
    int n = 0;

    LOG("FIELDS for %s:", macro);
    while ((fld = f_class_get_fields(klass, &it)) != NULL && n < 30) {
        const char* fn = f_field_get_name(fld);
        void* ft = f_field_get_type(fld);
        const char* tn = ft ? f_type_get_name(ft) : NULL;
        if (!fn) continue;
        LOG(" %s:%s", fn, tn ? tn : "?");
        n++;
    }
    LOG("\n");
}

static void log_bytes(const void* p, const char* macro)
{
    const unsigned char* b = (const unsigned char*)p;
    int k;
    LOG("BYTES %s:", macro);
    for (k = 0; k < 16; k++) LOG(" %02X", b[k]);
    LOG("\n");
}

static int name_match(const char* fn, const char* const* cands)
{
    int k;
    for (k = 0; k < 4; k++) if (cands[k][0] && strcmp(fn, cands[k]) == 0) return 1;
    for (k = 4; k < 8; k++) if (cands[k][0] && _stricmp(fn, cands[k]) == 0) return 1;
    return 0;
}

static int try_field(void* klass, const char* const* cands, int want, unsigned int* out)
{
    void* it = NULL;
    void* fld;
    int seen = 0;

    while ((fld = f_class_get_fields(klass, &it)) != NULL) {
        const char* fn = f_field_get_name(fld);
        if (!fn) continue;
        if (!name_match(fn, cands)) continue;
        if (f_field_get_flags(fld) & 0x10u) continue;
        if (++seen < want) continue;
        *out = (unsigned int)f_field_get_offset(fld);
        return 1;
    }
    return 0;
}

static int build_index(HMODULE ga)
{
    void* domain = NULL;
    const void** assemblies;
    size_t acount = 0, ai, i, total = 0;

    *(FARPROC*)&f_domain_get = GetProcAddress(ga, "il2cpp_domain_get");
    *(FARPROC*)&f_domain_get_assemblies = GetProcAddress(ga, "il2cpp_domain_get_assemblies");
    *(FARPROC*)&f_assembly_get_image = GetProcAddress(ga, "il2cpp_assembly_get_image");
    *(FARPROC*)&f_image_get_class_count = GetProcAddress(ga, "il2cpp_image_get_class_count");
    *(FARPROC*)&f_image_get_class = GetProcAddress(ga, "il2cpp_image_get_class");
    *(FARPROC*)&f_class_get_name = GetProcAddress(ga, "il2cpp_class_get_name");
    *(FARPROC*)&f_class_get_namespace = GetProcAddress(ga, "il2cpp_class_get_namespace");
    *(FARPROC*)&f_class_get_fields = GetProcAddress(ga, "il2cpp_class_get_fields");
    *(FARPROC*)&f_class_get_methods = GetProcAddress(ga, "il2cpp_class_get_methods");
    *(FARPROC*)&f_field_get_flags = GetProcAddress(ga, "il2cpp_field_get_flags");
    *(FARPROC*)&f_field_get_offset = GetProcAddress(ga, "il2cpp_field_get_offset");
    *(FARPROC*)&f_field_get_name = GetProcAddress(ga, "il2cpp_field_get_name");
    *(FARPROC*)&f_field_get_type = GetProcAddress(ga, "il2cpp_field_get_type");
    *(FARPROC*)&f_type_get_name = GetProcAddress(ga, "il2cpp_type_get_name");
    *(FARPROC*)&f_method_get_name = GetProcAddress(ga, "il2cpp_method_get_name");

    if (!f_domain_get || !f_domain_get_assemblies || !f_assembly_get_image ||
        !f_image_get_class_count || !f_image_get_class || !f_class_get_name ||
        !f_class_get_namespace || !f_class_get_fields || !f_class_get_methods ||
        !f_field_get_flags || !f_field_get_offset || !f_field_get_name ||
        !f_field_get_type || !f_type_get_name || !f_method_get_name)
    { LOG("critical export missing\n"); return 1; }

    for (i = 0; i < 600; i++) { domain = f_domain_get(); if (domain) break; Sleep(100); }
    if (!domain) { LOG("domain not initialised\n"); return 1; }

    assemblies = f_domain_get_assemblies(domain, &acount);
    if (!assemblies) { LOG("domain_get_assemblies failed\n"); return 1; }
    LOG("assemblies=%zu\n", acount);

    for (ai = 0; ai < acount; ai++) {
        void* img = f_assembly_get_image((void*)assemblies[ai]);
        if (img) total += f_image_get_class_count(img);
    }
    if (!total || total > AU_MAX) { LOG("class count %zu out of range\n", total); return 1; }

    g_all = (au_class*)calloc(total, sizeof(au_class));
    g_by_full = (size_t*)calloc(total, sizeof(size_t));
    g_by_short = (size_t*)calloc(total, sizeof(size_t));
    if (!g_all || !g_by_full || !g_by_short) { LOG("allocation failed\n"); return 1; }

    for (ai = 0; ai < acount; ai++) {
        void* img = f_assembly_get_image((void*)assemblies[ai]);
        size_t cnt, k;
        if (!img) continue;
        cnt = f_image_get_class_count(img);
        for (k = 0; k < cnt && g_n < total; k++) {
            void* klass = f_image_get_class(img, k);
            const char* nm;
            const char* ns;
            if (!klass) continue;
            nm = f_class_get_name(klass);
            if (!nm || !*nm) continue;
            ns = f_class_get_namespace(klass);
            g_all[g_n].klass = klass;
            if (ns && *ns) snprintf(g_all[g_n].full, AU_NAME, "%s.%s", ns, nm);
            else snprintf(g_all[g_n].full, AU_NAME, "%s", nm);
            snprintf(g_all[g_n].shortname, AU_SHORT, "%s", nm);
            g_n++;
        }
    }
    if (!g_n) { LOG("no classes indexed\n"); return 1; }

    for (i = 0; i < g_n; i++) { g_by_full[i] = i; g_by_short[i] = i; }
    qsort(g_by_full, g_n, sizeof(size_t), cmp_full);
    qsort(g_by_short, g_n, sizeof(size_t), cmp_short);
    LOG("indexed=%zu\n", g_n);
    return 0;
}

static int open_out(FILE** out)
{
    wchar_t dir[MAX_PATH], path[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
    if (!n || n >= MAX_PATH) return 1;
    wcscpy_s(path, MAX_PATH, dir);
    wcscat_s(path, MAX_PATH, L"\\RobRunInternal");
    CreateDirectoryW(path, NULL);
    wcscat_s(path, MAX_PATH, L"\\generated_offsets.h");
    _wfopen_s(out, path, L"w");
    return *out ? 0 : 1;
}

void rr_autoupdate_run(void)
{
    HMODULE ga;
    FILE* out = NULL;
    size_t i;
    int resolved = 0, unresolved = 0;

    if (InterlockedExchange(&g_started, 1)) return;
    ga = GetModuleHandleW(L"GameAssembly.dll");
    if (!ga) return;
    if (image_range()) return;

    {
        wchar_t dir[MAX_PATH], path[MAX_PATH];
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
        if (n && n < MAX_PATH) {
            wcscpy_s(path, MAX_PATH, dir);
            wcscat_s(path, MAX_PATH, L"\\RobRunInternal");
            CreateDirectoryW(path, NULL);
            wcscat_s(path, MAX_PATH, L"\\autoupdate.log");
            _wfopen_s(&g_log, path, L"w");
        }
    }
    LOG("rr_autoupdate base=%p size=0x%zX\n", g_base, g_size);

    if (build_index(ga)) { LOG("index failed\n"); return; }
    if (open_out(&out)) { LOG("cannot open output\n"); return; }

    fprintf(out, "/* GENERATED by rr_autoupdate - do not edit by hand. */\n");
    fprintf(out, "/* Entries marked UNRESOLVED kept their compiled value. */\n");
    fprintf(out, "#ifndef RR_GENERATED_OFFSETS_H\n#define RR_GENERATED_OFFSETS_H\n\n");

    for (i = 0; i < RR_AU_COUNT; i++) {
        const rr_au_row* r = &rr_au_rows[i];
        void* klass = NULL;
        unsigned int value = r->cur;
        char cbuf[4][160];
        char bbuf[4][176];
        const char* cands[8];
        char* hash;
        int want = 1, ok = 0, anycand = 0;

        if (r->kind == 3) {
            unresolved++;
            fprintf(out, "#define %s 0x%Xu  /* UNRESOLVED */\n", r->macro, r->cur);
            continue;
        }

        snprintf(cbuf[0], 160, "%s", r->member);
        hash = strrchr(cbuf[0], '#');
        if (hash) { want = atoi(hash + 1); *hash = 0; if (want < 1) want = 1; }
        snprintf(cbuf[1], 160, "%s", cbuf[0]);
        {
            size_t len = strlen(cbuf[1]);
            if (len > 2 && cbuf[1][len - 2] == '_' && cbuf[1][len - 1] == '1') cbuf[1][len - 2] = 0;
        }
        snprintf(cbuf[2], 160, "%s", cbuf[1] + (cbuf[1][0] == '_' ? 1 : 0));
        snprintf(cbuf[3], 160, "%s", cbuf[2]);
        if (cbuf[3][0] >= 'a' && cbuf[3][0] <= 'z') cbuf[3][0] = (char)(cbuf[3][0] - 32);
        {
            int k;
            for (k = 0; k < 4; k++) {
                snprintf(bbuf[k], 176, "<%s>k__BackingField", cbuf[k]);
                cands[k] = cbuf[k];
                cands[k + 4] = bbuf[k];
            }
        }

        if (r->kind == 0) {
            if (strchr(r->cls, '.')) {
                klass = resolve_class(r->cls);
                if (!klass) {
                    const char* dot = strrchr(r->cls, '.');
                    klass = resolve_class(dot + 1);
                }
                if (klass) {
                    anycand = 1;
                    ok = try_field(klass, cands, want, &value);
                    if (!ok) log_fields(klass, r->macro);
                }
            } else {
                size_t pos = lower_bound_short(r->cls);
                while (pos < g_n && strcmp(g_all[g_by_short[pos]].shortname, r->cls) == 0 && !ok) {
                    void* cand = g_all[g_by_short[pos]].klass;
                    anycand = 1;
                    if (!klass) klass = cand;
                    ok = try_field(cand, cands, want, &value);
                    if (!ok) log_fields(cand, r->macro);
                    pos++;
                }
            }
        } else {
            klass = resolve_class(r->cls);
            if (klass) {
                void* mit = NULL;
                void* m;
                anycand = 1;
                while ((m = f_class_get_methods(klass, &mit)) != NULL) {
                    const char* mn = f_method_get_name(m);
                    void* mp;
                    if (!mn || strcmp(mn, cands[0]) != 0) continue;
                    mp = *(void**)m;
                    if (!mp) continue;
                    if ((unsigned char*)mp < g_base || (unsigned char*)mp >= g_base + g_size) continue;
                    value = (unsigned int)((unsigned char*)mp - g_base);
                    log_bytes(mp, r->macro);
                    ok = 1;
                    break;
                }
            }
        }

        if (!anycand) {
            unresolved++;
            LOG("MISSING CLASS %s (%s)\n", r->cls, r->macro);
            fprintf(out, "#define %s 0x%Xu  /* UNRESOLVED class */\n", r->macro, r->cur);
            continue;
        }
        if (ok) { resolved++; fprintf(out, "#define %s 0x%Xu\n", r->macro, value); }
        else {
            unresolved++;
            LOG("UNRESOLVED %s (%s.%s)\n", r->macro, r->cls, r->member);
            fprintf(out, "#define %s 0x%Xu  /* UNRESOLVED member */\n", r->macro, r->cur);
        }
    }

    fprintf(out, "\n#endif\n");
    fclose(out);
    LOG("AUTOUPDATE_READY resolved=%d unresolved=%d\n", resolved, unresolved);
}
