#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "memory/rr_mem.h"
#include "rr_startup.h"

static wchar_t exe_directory[32768];
static FILE* status_file;
static int status_console;

void rr_status_open(void) {
    if (AttachConsole(ATTACH_PARENT_PROCESS) || GetLastError() == ERROR_ACCESS_DENIED)
        status_console = freopen("CONOUT$", "w", stdout) != NULL;
    DWORD length = GetModuleFileNameW(NULL, exe_directory, _countof(exe_directory));
    wchar_t* slash = length && length < _countof(exe_directory) ? wcsrchr(exe_directory, L'\\') : NULL;
    if (!slash) {
        exe_directory[0] = 0;
        return;
    }
    slash[1] = 0;
    wchar_t path[_countof(exe_directory)];
    if (swprintf_s(path, _countof(path), L"%lsstartup.log", exe_directory) > 0)
        status_file = _wfopen(path, L"a");
}

void rr_status(const char* format, ...) {
    if (!status_console && !status_file) return;
    SYSTEMTIME now;
    GetLocalTime(&now);
    char message[2048];
    va_list args;
    va_start(args, format);
    _vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);
    va_end(args);
    char line[2200];
    snprintf(line, sizeof(line), "< amphetamine > [ %04u-%02u-%02u ] [ %02u:%02u:%02u ] Status > %s\n",
             (unsigned)now.wYear, (unsigned)now.wMonth, (unsigned)now.wDay,
             (unsigned)now.wHour, (unsigned)now.wMinute, (unsigned)now.wSecond, message);
    if (status_console) {
        fputs(line, stdout);
        fflush(stdout);
    }
    if (status_file) {
        fputs(line, status_file);
        fflush(status_file);
    }
}

static void read_mapper_output(HANDLE pipe) {
    char chunk[2048], line[1024];
    DWORD received;
    size_t length = 0;
    while (ReadFile(pipe, chunk, sizeof(chunk), &received, NULL) && received) {
        for (DWORD i = 0; i < received; ++i) {
            const char c = chunk[i];
            if (c == '\n') {
                line[length] = 0;
                if (length && line[0] != '=') rr_status("%s", line);
                length = 0;
            } else if (c != '\r' && length < sizeof(line) - 1) {
                line[length++] = c;
            }
        }
    }
    if (length) {
        line[length] = 0;
        rr_status("%s", line);
    }
}

static HANDLE open_verified_image(void) {
    /* This exact image was associated with the 2026-11-07 crash. */
    static const BYTE rejected_hash[32] = {
        0xEC, 0x3A, 0x09, 0x68, 0x2C, 0x30, 0x07, 0x47,
        0xDC, 0x30, 0x29, 0xC4, 0xBA, 0xCA, 0x57, 0x28,
        0xB0, 0x14, 0x03, 0xB2, 0x0E, 0x21, 0x93, 0xA8,
        0x02, 0xBF, 0x80, 0xDB, 0xB1, 0x41, 0x66, 0xAA,
    };
    wchar_t path[_countof(exe_directory)];
    if (!exe_directory[0] ||
        swprintf_s(path, _countof(path), L"%lskernel-mode.sys", exe_directory) < 0)
        return INVALID_HANDLE_VALUE;
    HANDLE image = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (image == INVALID_HANDLE_VALUE) {
        rr_status("Could not open kernel-mode.sys beside the executable (Win32 %lu).", GetLastError());
        return image;
    }
    LARGE_INTEGER size;
    BYTE digest[sizeof(rejected_hash)];
    const DWORD max_image_size = 16 * 1024 * 1024;
    int verified = 0;
    if (GetFileSizeEx(image, &size) && size.QuadPart > 0 && size.QuadPart <= max_image_size) {
        DWORD length = (DWORD)size.QuadPart;
        BYTE* bytes = malloc(length);
        DWORD received;
        if (bytes && ReadFile(image, bytes, length, &received, NULL) && received == length &&
            BCryptHash(BCRYPT_SHA256_ALG_HANDLE, NULL, 0, bytes, length, digest, sizeof(digest)) == 0)
            verified = 1;
        free(bytes);
    }
    if (verified && memcmp(digest, rejected_hash, sizeof(digest)) != 0) return image;
    rr_status(verified ? "Rejected the driver image associated with the 2026-11-07 crash."
                       : "Could not verify the kernel-mode.sys image.");
    CloseHandle(image);
    return INVALID_HANDLE_VALUE;
}

static int run_mapper(void) {
    wchar_t mapper[_countof(exe_directory)];
    wchar_t command[] = L"kd2.exe --auto";
    if (!exe_directory[0] ||
        swprintf_s(mapper, _countof(mapper), L"%lskd2.exe", exe_directory) < 0 ||
        GetFileAttributesW(mapper) == INVALID_FILE_ATTRIBUTES) {
        rr_status("kd2.exe is missing beside the executable.");
        return 0;
    }

    SECURITY_ATTRIBUTES inherit = { sizeof(inherit), NULL, TRUE };
    HANDLE reader = NULL, writer = NULL;
    if (!CreatePipe(&reader, &writer, &inherit, 0)) {
        rr_status("Could not capture mapper output (Win32 %lu).", GetLastError());
        return 0;
    }
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               &inherit, OPEN_EXISTING, 0, NULL);
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = startup.hStdError = writer;
    BOOL started = input != INVALID_HANDLE_VALUE && SetHandleInformation(reader, HANDLE_FLAG_INHERIT, 0) &&
        CreateProcessW(mapper, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL,
                       exe_directory, &startup, &process);
    DWORD error = started ? ERROR_SUCCESS : GetLastError();
    CloseHandle(writer);
    if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
    if (!started) {
        CloseHandle(reader);
        rr_status("Could not launch kd2.exe (Win32 %lu).", error);
        return 0;
    }
    CloseHandle(process.hThread);
    read_mapper_output(reader);
    CloseHandle(reader);
    DWORD code = 1;
    if (WaitForSingleObject(process.hProcess, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(process.hProcess, &code)) code = 1;
    CloseHandle(process.hProcess);
    if (code) rr_status("Mapping failed (launcher exit %lu).", code);
    return code == 0;
}

static int read_driver_status(const wchar_t* name, DWORD* value) {
    DWORD size = sizeof(*value);
    return RegGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\SmxRing", name, RRF_RT_REG_DWORD,
                        NULL, value, &size) == ERROR_SUCCESS;
}

static int wait_for_driver(void) {
    for (int attempt = 0; attempt < 40; ++attempt) {
        DWORD status;
        if (read_driver_status(L"InitStatus", &status) && status) {
            rr_status("SmxRing initialization failed (0x%08lX). Restart Windows before retrying.", status);
            return 0;
        }
        if (rr_drv_probe()) {
            rr_status("Driver mapped and responding.");
            return 1;
        }
        Sleep(250);
    }
    rr_status("SmxRing did not respond within 10 seconds.");
    return 0;
}

int rr_startup_driver(void) {
    HANDLE lock = CreateMutexW(NULL, FALSE, L"Local\\RRToolSmxRingStartup");
    if (!lock) {
        rr_status("Could not create mapper lock (Win32 %lu).", GetLastError());
        return 0;
    }
    DWORD wait = WaitForSingleObject(lock, 0);
    if (wait == WAIT_TIMEOUT) {
        rr_status("Waiting for another launch to finish its driver check...");
        wait = WaitForSingleObject(lock, INFINITE);
    }
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) {
        rr_status("Could not acquire the mapper lock.");
        CloseHandle(lock);
        return 0;
    }
    rr_status("Checking kernel driver...");
    int ready = rr_drv_probe();
    if (ready) {
        rr_status("Driver is already mapped and responding.");
    } else {
        DWORD status, token;
        if (read_driver_status(L"InitStatus", &status) && status) {
            rr_status("SmxRing initialization failed (0x%08lX). Restart Windows before retrying.", status);
        } else if (read_driver_status(L"RingToken", &token) && token) {
            ready = wait_for_driver();
        } else {
            HANDLE image = open_verified_image();
            if (image != INVALID_HANDLE_VALUE) {
                rr_status("Mapping driver...");
                if (run_mapper()) ready = wait_for_driver();
                CloseHandle(image);
            }
        }
    }
    ReleaseMutex(lock);
    CloseHandle(lock);
    return ready;
}
