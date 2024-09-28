#pragma once
#include <cstdint>

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

// Member annotation to mark members handled by custom repacking. This enables
// automatic struct repacking of structs with non-trivial members (pointers,
// unions, ...). Repacking logic is auto-generated as usual, with the
// difference that an external function is called to manually repack the
// annotated members.
//
// Two functions must be implemented for the parent struct type:
// * fex_custom_repack_entry, called after automatic repacking of the other members
// * fex_custom_repack_exit, called on exit but before automatic exit-repacking
//     of the other members. Non-trivial implementations must perform host->guest
//     repacking manually and return the boolean value true.
//
// If multiple members of the same struct are annotated as custom_repack,
// they must be handled in the same fex_custom_repack_entry/exit functions.
struct custom_repack {};

// Type annotation to indicate that guest_layout/host_layout definitions should
// be emitted even if the type is non-repackable. Pointer members will be
// copied (or zero-extended) without regard for the referred data.
struct emit_layout_wrappers {};

struct type_annotation_base {
  bool prevent_multiple;
};

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

struct inregister_abi {};

} // namespace fexgen
