// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <sstream>

namespace fextl {
  template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
  using basic_stringbuf = std::basic_stringbuf<CharT, Traits, Allocator>;

  template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
  using basic_istringstream = std::basic_istringstream<CharT, Traits, Allocator>;

  template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
  using basic_ostringstream = std::basic_ostringstream<CharT, Traits, Allocator>;

  template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = fextl::FEXAlloc<CharT>>
  using basic_stringstream = std::basic_stringstream<CharT, Traits, Allocator>;

  using stringbuf = fextl::basic_stringbuf<char>;
  using istringstream = fextl::basic_istringstream<char>;
  using ostringstream = fextl::basic_ostringstream<char>;
  using stringstream = fextl::basic_stringstream<char>;
}
