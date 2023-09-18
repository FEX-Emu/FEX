// SPDX-License-Identifier: MIT
#pragma once

#ifndef DO_PRAGMA
#define DO_PRAGMA(x) _Pragma (#x)
#endif

#if FEX_WARN_TODO
// FEX_TODO_ISSUE(github ticket number, "comment")
#define FEX_TODO_ISSUE(github_ticket, comment) DO_PRAGMA(GCC warning "TODO: https://github.com/FEX-Emu/FEX/issues/" #github_ticket comment);
// FEX_TODO("comment")
#define FEX_TODO(comment) DO_PRAGMA(GCC warning "TODO: " comment);
#else
// FEX_TODO_ISSUE(github ticket number, "comment")
#define FEX_TODO_ISSUE(github_ticket, comment)
// FEX_TODO("comment")
#define FEX_TODO(comment)
#endif

// For linking to tickets, non-todo
// FEX_TICKET(github ticket number) or FEX_TICKET(github ticket number, "comment")
#define FEX_TICKET(github_ticket, ...)