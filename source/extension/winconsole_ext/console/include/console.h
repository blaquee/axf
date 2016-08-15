#pragma once

// Windows
#include <Windows.h>

#ifdef CONSOLE_DYNAMIC
#ifdef CONSOLE_EXPORTS
#define CONSOLE_API __declspec(dllexport)
#else
#define CONSOLE_API __declspec(dllimport)
#endif
#else
#define CONSOLE_API
#endif

// Opaque console type
typedef struct CONSOLE CONSOLE;

CONSOLE_API int  console_create(CONSOLE** console);
CONSOLE_API void console_show(CONSOLE* console, int visible);
CONSOLE_API int  console_visible(CONSOLE* console);
CONSOLE_API void console_toggle(CONSOLE* console);
CONSOLE_API int  console_write(CONSOLE* console, const wchar_t* text);
CONSOLE_API int  console_write_utf8(CONSOLE* console, const char* text);
CONSOLE_API int  console_read(CONSOLE* console, const wchar_t* buffer, size_t length);
CONSOLE_API int  console_destroy(CONSOLE* console);