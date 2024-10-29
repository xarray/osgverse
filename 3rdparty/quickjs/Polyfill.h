//
// Created by qingy on 2024/7/19.
//

#pragma once
#if defined(_MSC_VER)
#include <intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <direct.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN

#define __attribute__(x)
#define __attribute(x)
#define PATH_MAX MAX_PATH
#define CONFIG_VERSION "quickjs"

#if _MSC_VER <= 1900
#   define INT64_MAX_BIN 9e18
#else
#   define INT64_MAX_BIN 0x1p63
#endif

int  __inline pclose(FILE* h) {
    return _pclose(h);
}

__inline FILE* popen(const char* command, const char* mode) {
    return _popen(command, mode);
}

int __inline chdir(const char* path) {
    return _chdir(path);
}

int __inline open(const char* filename, int oflag, int pmode) {
    return _open(filename, oflag, pmode);
}

int __inline close(int handle) {
    return _close(handle);
}

long __inline lseek(int fd, long offset, int origin) {
    return _lseek(fd, offset, origin);
}

int __inline write(int fd, const void* buffer, unsigned int count) {
    return _write(fd, buffer, count);
}

int __inline read(int const fd, void* const buffer, unsigned const buffer_size) {
    return _read(fd, buffer, buffer_size);
}

int __inline isatty(int fd) {
    return _isatty(fd);
}

// from https://www.davekb.com/browse_programming_tips:windows_isreg_isdir:txt
#ifndef _S_ISTYPE
#define _S_ISTYPE(mode, mask)  (((mode) & _S_IFMT) == (mask))
#define S_ISREG(mode) _S_ISTYPE((mode), _S_IFREG)
#define S_ISDIR(mode) _S_ISTYPE((mode), _S_IFDIR)
#endif


// from https://github.com/binzume/quickjs-msvc/blob/master/quickjs-libc.c

struct dirent {
    int d_ino;
    int d_type;
    char d_name[MAX_PATH];
};
typedef struct DIR {
    char path[MAX_PATH];
    struct dirent dir;
    WIN32_FIND_DATA w32FindData;
    HANDLE hFile;
} DIR;

__inline DIR* opendir(const char* path) {
    DIR* d = (DIR*)malloc(sizeof(DIR));
    strcpy(d->path, path);
    strcat(d->path, "\\*");
    d->hFile = FindFirstFile(d->path, &d->w32FindData);
    if (d->hFile == INVALID_HANDLE_VALUE) {
        d->hFile = NULL;
        return NULL;
    }
    return d;
}

int __inline closedir(DIR* d) {
    if (!d) return 0;
    if (d->hFile) FindClose(d->hFile);
    free(d);
    return 1;
}

__inline struct dirent* readdir(DIR* d) {
    if (!d->hFile) return NULL;
    d->dir.d_type = d->w32FindData.dwFileAttributes;
    d->dir.d_ino = 1;
    strcpy(d->dir.d_name, d->w32FindData.cFileName);
    if (!FindNextFile(d->hFile, &d->w32FindData)) {
        FindClose(d->hFile);
        d->hFile = NULL;
    }
    return &d->dir;
}


// from https://stackoverflow.com/a/26085827
#if !defined(_WINSOCKAPI_)
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;
#endif

int __inline gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    return 0;
}

// from https://stackoverflow.com/q/355967

uint32_t __inline __builtin_ctz(uint32_t value) {
    return _tzcnt_u32(value);
}

uint32_t __inline __builtin_clz(uint32_t value) {
    return _lzcnt_u32(value);
}

uint32_t __inline __builtin_ctzll(uint64_t value) {
    return _tzcnt_u64(value);
}

uint32_t __inline __builtin_clzll(uint64_t value) {
    return _lzcnt_u64(value);
}

typedef long ssize_t;

#else

#define CONFIG_VERSION "quickjs"
#define INT64_MAX_BIN 0x1p63

#endif
