namespace fexgen {
struct returns_guest_pointer {};
struct custom_host_impl {};
struct custom_guest_entrypoint {};

struct generate_guest_symtable {};

struct callback_annotation_base {
    // Prevent annotating multiple callback strategies
    bool prevent_multiple;
};
struct callback_stub : callback_annotation_base {};
struct callback_guest : callback_annotation_base {};

struct with_variadic_strategy {
    // Prevent annotating multiple variadic strategies
    bool prevent_multiple_variadic;
};

// Functions with a va_list that is parsed according to a format string
struct like_printf : with_variadic_strategy {};

// Similar to like_printf, but for functions that take a heap buffer as target output
template<int BufferIdx>
struct like_sprintf : with_variadic_strategy {};

// Similar to like_printf, but for functions that take a sized buffer as target output
template<int BufferIdx, int CountIdx>
struct like_snprintf : with_variadic_strategy {};

} // namespace fexgen
