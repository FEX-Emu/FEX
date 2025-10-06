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
#endif
