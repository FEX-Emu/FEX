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

// If used, fex_custom_repack must be specialized for the annotated struct member
struct custom_repack {};

struct type_annotation_base { bool prevent_multiple; };

// Pointers to types annotated with this will be passed through without change
struct opaque_type : type_annotation_base {};

// Function parameter annotation.
// Pointers are passed through to host (extending to 64-bit if needed) without modifying the pointee.
// The type passed to Host is guest_layout<PointeeType>*.
// TODO: Update description. "The raw guest_layout<type>* will be passed to the host"
struct ptr_passthrough {};

// Type / Function parameter annotation.
// Assume objects of the given type are compatible across architectures,
// even if the generator can't automatically prove this. For pointers, this refers to the pointee type.
// NOTE: In contrast to opaque_type, this allows for non-pointer members with the annotated type to be repacked automatically.
struct assume_compatible_data_layout : type_annotation_base {};

} // namespace fexgen
