// SPDX-License-Identifier: MIT
#pragma once

#ifndef _WIN32
#include <linux/prctl.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/prctl.h>

#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#endif

#ifndef PR_SET_VMA_ANON_NAME
#define PR_SET_VMA_ANON_NAME 0
#endif

#ifndef PR_GET_MEM_MODEL
#define PR_GET_MEM_MODEL 0x6d4d444c
#endif
#ifndef PR_SET_MEM_MODEL
#define PR_SET_MEM_MODEL 0x4d4d444c
#endif
#ifndef PR_SET_MEM_MODEL_DEFAULT
#define PR_SET_MEM_MODEL_DEFAULT 0
#endif
#ifndef PR_SET_MEM_MODEL_TSO
#define PR_SET_MEM_MODEL_TSO 1
#endif

#ifndef PR_GET_COMPAT_INPUT
#define PR_GET_COMPAT_INPUT 0x63494e50
#endif
#ifndef PR_SET_COMPAT_INPUT
#define PR_SET_COMPAT_INPUT 0x43494e50
#endif
#ifndef PR_SET_COMPAT_INPUT_DISABLE
#define PR_SET_COMPAT_INPUT_DISABLE 0
#endif
#ifndef PR_SET_COMPAT_INPUT_ENABLE
#define PR_SET_COMPAT_INPUT_ENABLE 1
#endif

#ifndef PR_GET_SHADOW_STACK_STATUS
#define PR_GET_SHADOW_STACK_STATUS 74
#endif
#ifndef PR_LOCK_SHADOW_STACK_STATUS
#define PR_LOCK_SHADOW_STACK_STATUS 76
#endif
#ifndef PR_SHADOW_STACK_ENABLE
#define PR_SHADOW_STACK_ENABLE (1ULL << 0)
#endif

#endif // ifndef _WIN32
