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

// Type annotation to indicate that guest_layout/host_layout definitions should
// be emitted even if the type is non-repackable. Pointer members will be
// copied (or zero-extended) without regard for the referred data.
struct emit_layout_wrappers {};

struct type_annotation_base { bool prevent_multiple; };

// Pointers to types annotated with this will be passed through without change
struct opaque_type : type_annotation_base {};

// Function parameter annotation.
// Pointers are passed through to host (extending to 64-bit if needed) without modifying the pointee.
// The type passed to Host will be guest_layout<pointee_type>*.
struct ptr_passthrough {};

// Type / Function parameter annotation.
// Assume objects of the given type are compatible across architectures,
// even if the generator can't automatically prove this. For pointers, this refers to the pointee type.
// NOTE: In contrast to opaque_type, this allows for non-pointer members with the annotated type to be repacked automatically.
struct assume_compatible_data_layout : type_annotation_base {};

} // namespace fexgen
