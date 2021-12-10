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

} // namespace fexgen
