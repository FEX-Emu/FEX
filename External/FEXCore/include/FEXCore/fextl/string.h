#pragma once
#include <FEXCore/fextl/allocator.h>

#include <filesystem>
#include <string>

namespace fextl {
  template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
  using basic_string = std::basic_string<CharT, Traits, Allocator>;

  using string = fextl::basic_string<char>;
}

template<>
 struct std::hash<fextl::string> {
  std::size_t operator()(fextl::string const& s) const noexcept {
    return std::hash<std::string_view>{}(s);
  };
};
