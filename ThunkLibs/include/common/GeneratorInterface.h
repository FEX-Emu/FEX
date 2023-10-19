namespace fexgen {
struct returns_guest_pointer {};
struct custom_host_impl {};
struct custom_guest_entrypoint {};

struct generate_guest_symtable {};
struct indirect_guest_calls {};

struct callback_annotation_base {
    // Prevent annotating multiple callback strategies
    bool prevent_multiple;
};
struct callback_stub : callback_annotation_base {};
struct callback_guest : callback_annotation_base {};

// Function parameter annotation.
// Pointers are passed through to host (extending to 64-bit if needed) without modifying the pointee.
// The type passed to Host will be guest_layout<pointee_type>*.
struct ptr_passthrough {};

} // namespace fexgen
