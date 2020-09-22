#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <GL/glx.h>
#include <dlfcn.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>
#include <stdarg.h>

#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

LOAD_LIB(libSDL2)

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

extern "C" {
    void* SDL_GL_GetProcAddress(const char* name) {
		// TODO: Fix this HACK
		return (void*)glXGetProcAddress((const GLubyte*)name);
    }

    // TODO: These are not 100% conforming to SDL either
    void *SDL_LoadObject(const char *sofile) {
        auto lib = dlopen(sofile, RTLD_NOW | RTLD_LOCAL);
        if (!lib) {
            printf("SDL_LoadObject: Failed to load %s\n", sofile);
        }
        return lib;
    }

    void *SDL_LoadFunction(void *lib, const char *name) {
        return dlsym(lib, name);
    }

    void SDL_UnloadObject(void *lib) {
        if (lib) {
            dlclose(lib);
        }
    }
}