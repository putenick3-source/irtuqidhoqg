/* rr_buildid.c - the game patches weekly and silently shifts .data and field
 * offsets. A stale offset set shows up as empty class names and a zeroed
 * dictionary primitive offset, which reads like a code bug. This refuses to
 * pretend: the expected image is pinned by size and SHA-256. Re-run the
 * generator after every game patch. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/rr_buildid.h"

#define EXPECTED_SIZE 148323840ULL
static const unsigned char expected_hash[32] = { 0x2D, 0xC3, 0x32, 0x69, 0x15, 0xEB, 0x1E, 0x62, 0xE8, 0x6D, 0xEE, 0x32, 0xD9, 0xFD, 0x64, 0xF8, 0x02, 0x06, 0x23, 0xF6, 0x22, 0x0D, 0xE7, 0x47, 0x27, 0x2C, 0xD7, 0xAD, 0xE7, 0x67, 0xB5, 0xCF };

static void log_line(const char* text) {
    wchar_t dir[MAX_PATH], path[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
    if (!n || n >= MAX_PATH) return;
    if (swprintf_s(path, MAX_PATH, L"%ls\\RobRunInternal", dir) < 0) return;
    CreateDirectoryW(path, NULL);
    if (swprintf_s(path, MAX_PATH, L"%ls\\RobRunInternal\\buildid.log", dir) < 0) return;
    FILE* f = _wfopen(path, L"a");
    if (!f) return;
    fputs(text, f);
    fputc('\n', f);
    fclose(f);
}

static void to_hex(const unsigned char* in, size_t n, char* out) {
    static const char d[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; ++i) { out[i * 2] = d[in[i] >> 4]; out[i * 2 + 1] = d[in[i] & 15]; }
    out[n * 2] = 0;
}

int rr_buildid_check(char* msg, size_t cap) {
    char line[512];
    HMODULE ga = GetModuleHandleW(L"GameAssembly.dll");
    wchar_t path[MAX_PATH];
    LARGE_INTEGER size;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    unsigned char digest[32];
    char actual[65], expected[65];
    int ok = 0;
    HANDLE file;

    if (!ga) { log_line("buildid: GameAssembly.dll not loaded"); return 0; }
    if (!GetModuleFileNameW(ga, path, MAX_PATH)) { log_line("buildid: path unresolved"); return 0; }
    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) { log_line("buildid: cannot open the image"); return 0; }
    if (!GetFileSizeEx(file, &size)) { CloseHandle(file); return 0; }

    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0 &&
        BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0) == 0) {
        unsigned char* buf = malloc(1 << 20);
        DWORD got = 0;
        ok = buf != NULL;
        while (ok && ReadFile(file, buf, 1 << 20, &got, NULL) && got)
            ok = BCryptHashData(hash, buf, got, 0) == 0;
        free(buf);
        if (ok) ok = BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0;
    }
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(file);
    if (!ok) { log_line("buildid: hashing failed"); return 0; }

    to_hex(digest, sizeof(digest), actual);
    to_hex(expected_hash, sizeof(expected_hash), expected);

    if ((unsigned long long)size.QuadPart == EXPECTED_SIZE &&
        memcmp(digest, expected_hash, sizeof(digest)) == 0) {
        snprintf(line, sizeof(line), "buildid: OK %s (%llu bytes)", actual, (unsigned long long)size.QuadPart);
        log_line(line);
        if (msg && cap) snprintf(msg, cap, "GameAssembly.dll matches");
        return 1;
    }
    snprintf(line, sizeof(line),
             "buildid: MISMATCH actual=%s size=%llu expected=%s size=%llu - offsets are stale",
             actual, (unsigned long long)size.QuadPart, expected, EXPECTED_SIZE);
    log_line(line);
    if (msg && cap) snprintf(msg, cap, "GameAssembly.dll %s does not match the expected %s", actual, expected);
    return -1;
}