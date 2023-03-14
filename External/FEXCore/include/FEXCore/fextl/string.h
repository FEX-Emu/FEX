#pragma once
#include <fextl/allocator.h>

#include <string>

namespace fextl {
  template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
  using basic_string = std::basic_string<CharT, Traits, Allocator>;

  using string = fextl::basic_string<char>;
}
