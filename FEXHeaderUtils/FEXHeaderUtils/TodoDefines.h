// SPDX-License-Identifier: MIT
#pragma once

#ifndef DO_PRAGMA
#define DO_PRAGMA(x) _Pragma(#x)
#endif

#if FEX_WARN_TODO
// Use like FEX_TODO_ISSUE(github ticket number, username, comment) or
// FEX_TODO_ISSUE(github ticket number, comment)
#define FEX_TODO_ISSUE(number, ...) DO_PRAGMA(GCC warning "TODO: https://github.com/FEX-Emu/FEX/issues/" #number __VA_ARGS__)
// Use like FEX_TODO(username, comment) or FEX_TODO(comment)
#define FEX_TODO(...) DO_PRAGMA(GCC warning "TODO: " __VA_ARGS__);
#else
// Use like FEX_TODO_ISSUE(github ticket number, username, comment) or
// FEX_TODO_ISSUE(github ticket number, comment)
#define FEX_TODO_ISSUE(number, ...) do {} while (false)
// Use like FEX_TODO(username, comment) or FEX_TODO(comment)
#define FEX_TODO(...) do {} while (false)
#endif
