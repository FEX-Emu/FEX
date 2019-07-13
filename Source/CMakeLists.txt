if (ENABLE_CLANG_FORMAT)
  find_program(CLANG_TIDY_EXE "clang-tidy")
  set(CLANG_TIDY_FLAGS
    "-checks=*"
    "-fuchsia*"
    "-bugprone-macro-parentheses"
    "-clang-analyzer-core.*"
    "-cppcoreguidelines-pro-type-*"
    "-cppcoreguidelines-pro-bounds-array-to-pointer-decay"
    "-cppcoreguidelines-pro-bounds-pointer-arithmetic"
    "-cppcoreguidelines-avoid-c-arrays"
    "-cppcoreguidelines-avoid-magic-numbers"
    "-cppcoreguidelines-pro-bounds-constant-array-index"
    "-cppcoreguidelines-no-malloc"
    "-cppcoreguidelines-special-member-functions"
    "-cppcoreguidelines-owning-memory"
    "-cppcoreguidelines-macro-usage"
    "-cppcoreguidelines-avoid-goto"
    "-google-readability-function-size"
    "-google-readability-namespace-comments"
    "-google-readability-braces-around-statements"
    "-google-build-using-namespace"
    "-hicpp-*"
    "-llvm-namespace-comment"
    "-llvm-include-order" # Messes up with case sensitivity
    "-misc-unused-parameters"
    "-modernize-loop-convert"
    "-modernize-use-auto"
    "-modernize-avoid-c-arrays"
    "-modernize-use-nodiscard"
    "readability-*"
    "-readability-function-size"
    "-readability-implicit-bool-conversion"
    "-readability-braces-around-statements"
    "-readability-else-after-return"
    "-readability-magic-numbers"
    "-readability-named-parameter"
    "-readability-uppercase-literal-suffix"
    "-cert-err34-c"
    "-cert-err58-cpp"
    "-bugprone-exception-escape"
    )
  string(REPLACE ";" "," CLANG_TIDY_FLAGS "${CLANG_TIDY_FLAGS}")

  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} "${CLANG_TIDY_FLAGS}")
endif()


add_subdirectory(Common/)
add_subdirectory(CommonCore/)
add_subdirectory(Tests/)
add_subdirectory(Tools/)
