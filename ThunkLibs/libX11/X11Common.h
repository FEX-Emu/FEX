#pragma once

#include <array>
#include <string>
extern "C" {
#include <X11/Xlib.h>
}

namespace X11 {
  constexpr static std::array<std::string_view, 13> CallbackKeys = {{
    XNGeometryCallback,
    XNDestroyCallback,
    XNPreeditStartCallback,
    XNPreeditDoneCallback,
    XNPreeditDrawCallback,
    XNPreeditCaretCallback,
    XNPreeditStateNotifyCallback,
    XNStatusStartCallback,
    XNStatusDoneCallback,
    XNStatusDrawCallback,
    XNR6PreeditCallback,
    XNStringConversionCallback,
  }};

}
