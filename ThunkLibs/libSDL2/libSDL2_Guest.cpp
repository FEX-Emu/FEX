#include <SDL2/SDL.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "../Thunk.h"
#include <stdarg.h>

LOAD_LIB(libSDL2)

#include "libSDL2_Thunks.inl"
#include <vector>

struct __va_list_tag;

int SDL_snprintf(char*, size_t, const char*, ...) { return printf("SDL2: SDL_snprintf\n"); }
int SDL_sscanf(const char*, const char*, ...) { return printf("SDL2: SDL_sscanf\n"); }
void SDL_Log(const char*, ...) { printf("SDL2: SDL_Log\n"); }
void SDL_LogCritical(int, const char*, ...) { printf("SDL2: SDL_LogCritical\n"); }
void SDL_LogDebug(int, const char*, ...) { printf("SDL2: SDL_LogDebug\n"); }
void SDL_LogError(int, const char*, ...) { printf("SDL2: SDL_LogError\n"); }
void SDL_LogInfo(int, const char*, ...) { printf("SDL2: SDL_LogInfo\n"); }
void SDL_LogMessage(int, SDL_LogPriority, const char*, ...) { printf("SDL2: SDL_LogMessage\n"); }
void SDL_LogVerbose(int, const char*, ...) { printf("SDL2: SDL_LogVerbose\n"); }
void SDL_LogWarn(int, const char*, ...) { printf("SDL2: SDL_LogWarn\n"); }
int SDL_SetError(const char*, ...) { return printf("SDL2: SDL_SetError\n"); }

void SDL_LogMessageV(int, SDL_LogPriority, const char*, __va_list_tag*) { printf("SDL2: SDL_LogMessageV\n");}
int SDL_vsnprintf(char*, size_t, const char*, __va_list_tag*) { return printf("SDL2: SDL_vsnprintf\n");}
int SDL_vsscanf(const char*, const char*, __va_list_tag*) { return printf("SDL2: SDL_vsscanf\n");}