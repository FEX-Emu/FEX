// SPDX-License-Identifier: MIT
#define _SECIMP
#define _CRTIMP


#define vsprintf __ignore__vsprintf
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <wchar.h>
#include <time.h>
#include "../Priv.h"
#undef vsprintf

extern "C" int __cdecl vsprintf(char* __restrict__ _Dest, const char* __restrict__ _Format, va_list _Args) __MINGW_ATTRIB_DEPRECATED_SEC_WARN;

static unsigned short CTypeData[256];
static char Locale[2] = "C";

DLLEXPORT_FUNC(char*, _strdup, (const char* Src)) {
  size_t Len = strlen(Src) + 1;
  char* Dst = reinterpret_cast<char*>(malloc(Len));
  memcpy(Dst, Src, Len);
  return Dst;
}

char* strdup(const char* Src) {
  return _strdup(Src);
}

float strtof(const char* __restrict__, char** __restrict__) {
  UNIMPLEMENTED();
}

double strtod(const char* __restrict__, char** __restrict__) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(double, _strtod_l, (const char* __restrict__ _Str, char** __restrict__ _EndPtr, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

long long wcstoll(const wchar_t* __restrict__ nptr, wchar_t** __restrict__ endptr, int base) {
  UNIMPLEMENTED();
}

unsigned long long wcstoull(const wchar_t* __restrict__ nptr, wchar_t** __restrict__ endptr, int base) {
  UNIMPLEMENTED();
}

long long atoll(const char*) {
  UNIMPLEMENTED();
}

long long strtoll(const char* __restrict__, char** __restrict, int) {
  UNIMPLEMENTED();
}

long double strtold(const char* __restrict__, char** __restrict__) {
  UNIMPLEMENTED();
}

double wcstod(const wchar_t* __restrict__ _Str, wchar_t** __restrict__ _EndPtr) {
  UNIMPLEMENTED();
}

long double wcstold(const wchar_t* __restrict__, wchar_t** __restrict__) {
  UNIMPLEMENTED();
}

float wcstof(const wchar_t* __restrict__ nptr, wchar_t** __restrict__ endptr) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(__int64, _strtoi64_l, (const char* _String, char** _EndPtr, int _Radix, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(unsigned __int64, _strtoui64_l, (const char* _String, char** _EndPtr, int _Radix, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

int __stdio_common_vsscanf(unsigned __int64 options, const char* input, size_t length, const char* format, _locale_t locale, va_list valist) {
  UNIMPLEMENTED();
}

int __stdio_common_vswprintf(unsigned __int64 options, wchar_t* str, size_t len, const wchar_t* format, _locale_t locale, va_list valist) {
  return _vsnwprintf(str, len, format, valist);
}

int __mingw_vsnwprintf(wchar_t* __restrict__ Dest, size_t Count, const wchar_t* __restrict__ Format, va_list Args) {
  int ret = _vsnwprintf(Dest, Count, Format, Args);
  return ret;
}

int __mingw_vsprintf(char* __restrict__ Dest, const char* __restrict__ Format, va_list Args) {
  int ret = vsprintf(Dest, Format, Args);
  return ret;
}

DLLEXPORT_FUNC(size_t, _strftime_l,
               (char* __restrict__ Buf, size_t Max_size, const char* __restrict__ Format, const struct tm* __restrict__ Tm, _locale_t Locale)) {
  UNIMPLEMENTED();
}

int vsnprintf(char* __restrict__ Dest, size_t Count, const char* __restrict__ Format, va_list Args) {
  int ret = _vsnprintf(Dest, Count, Format, Args);
  if (ret == -1) {
    Dest[Count - 1] = '\0';
    return _vsnprintf(nullptr, 0, Format, Args);
  }
  return ret;
}

int snprintf(char* stream, size_t n, const char* format, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, format);
  int ret = vsnprintf(stream, n, format, args);
  __builtin_va_end(args);
  return ret;
}

char* setlocale(int _Category, const char* _Locale) {
  return Locale;
}

int _configthreadlocale(int _Flag) {
  return 0;
}

DLLEXPORT_FUNC(_locale_t, _create_locale, (int _Category, const char* _Locale)) {
  return nullptr;
}

DLLEXPORT_FUNC(struct lconv*, localeconv, (void)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(void, _free_locale, (_locale_t _Locale)) {}

wint_t btowc(int) {
  UNIMPLEMENTED();
}

size_t mbsrtowcs(wchar_t* __restrict__ _Dest, const char** __restrict__ _PSrc, size_t _Count,
                 mbstate_t* __restrict__ _State) __MINGW_ATTRIB_DEPRECATED_SEC_WARN {
  UNIMPLEMENTED();
}

size_t mbrtowc(wchar_t* __restrict__ _DstCh, const char* __restrict__ _SrcCh, size_t _SizeInBytes, mbstate_t* __restrict__ _State) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _mbtowc_l, (wchar_t* __restrict__ DstCh, const char* __restrict__ SrcCh, size_t SrcSizeInBytes, _locale_t Locale)) {
  if (!SrcCh || SrcSizeInBytes == 0) {
    return 0;
  }
  *DstCh = static_cast<wchar_t>(*SrcCh);
  return 1;
}

size_t mbrlen(const char* __restrict__ _Ch, size_t _SizeInBytes, mbstate_t* __restrict__ _State) {
  UNIMPLEMENTED();
}

size_t wcrtomb(char* __restrict__ _Dest, wchar_t _Source, mbstate_t* __restrict__ _State) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(errno_t, wcrtomb_s, (size_t * _Retval, char* _Dst, size_t _SizeInBytes, wchar_t _Ch, mbstate_t* _State)) {
  UNIMPLEMENTED();
}

int wctob(wint_t _WCh) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(errno_t, strerror_s, (char* _Buf, size_t _SizeInBytes, int _ErrNum)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _isctype, (int _C, int _Type)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(const unsigned short*, __pctype_func, (void)) {
  return CTypeData;
}

DLLEXPORT_FUNC(int, _isctype_l, (int _C, int _Type, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _strcoll_l, (const char* _Str1, const char* _Str2, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(size_t, _strxfrm_l, (char* __restrict__ _Dst, const char* __restrict__ _Src, size_t _MaxCount, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _wcscoll_l, (const wchar_t* _Str1, const wchar_t* _Str2, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(size_t, _wcsxfrm_l, (wchar_t* __restrict__ _Dst, const wchar_t* __restrict__ _Src, size_t _MaxCount, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswalpha_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswupper_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswlower_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswdigit_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswxdigit_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswspace_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswpunct_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswalnum_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswprint_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswgraph_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _iswcntrl_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(wint_t, _towupper_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(wint_t, _towlower_l, (wint_t _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _toupper_l, (int _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, _tolower_l, (int _C, _locale_t _Locale)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int, ___mb_cur_max_func, (void)) {
  UNIMPLEMENTED();
}
