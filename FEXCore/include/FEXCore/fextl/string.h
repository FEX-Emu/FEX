// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <functional>
#include <string>

namespace fextl {
template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
using basic_string = std::basic_string<CharT, Traits, Allocator>;

using string = fextl::basic_string<char>;
} // namespace fextl

template<>
struct std::hash<fextl::string> {
  std::size_t operator()(const fextl::string& s) const noexcept {
    return std::hash<std::string_view> {}(s);
  };
};
