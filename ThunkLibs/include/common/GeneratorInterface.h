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


//Taxonomy of pointer annotations:
//* input/output/both?
//* is array? (size needs to be known if ABI must be fixed up)
//* is null-terminated string? (E.g. filename used for stat)
//* deals with memory allocation (factory functions, malloc/free, strdup)
//
//* Temporal local uses: Pointer is only used while executing the function (not stored after returning)
//  * Probably best to be implied, and annotation should be required for exceptions
//
//* Nonaliased: Not aliased by other pointers of this type (equivalent to "restrict")
//
//* passthrough: Not used by the actual library (e.g. _ftsent::fts_pointer), hence doesn't even require 32->64 bit extension
//* untyped address: References an address in memory where the host never accesses the data itself (e.g. free, stack_t::ss_sp). Guest-pointer can be used as host-pointer via zero-extension hence
//  * Example: free (first argument), malloc (return value)
//* opaque ptr: Addressed data is accesssed only indirectly via the library interface. Same effect as untyped address
//  * opaque_to_guest: Zero-extend pointer to host-size (Example: Most constructor-like functions in object-oriented C APIs)
//  * opaque_to_host: Same as opaque_to_guest, but host-side type can be an actual opaque type wrapper (e.g. user-data pointers for callbacks)
//
//* [if used in the wild] must preserve pointer location
//  * e.g. if pointer is used as used as index to a map
//  * or if pointer points to element within struct (`struct { int a; int* b = &a; }`)
//* Implied alternative: Value-pointer (i.e. only the data matters, not the address)

// Pointer guide:
// * Don't annotate pointers to ABI-compatible types
//   * TODO: Instead, require annotation with ptr_in/out/inout? (could make an exception for const pointers)
// * If applicable, use one of { passthrough, untyped_address, opaque_to_guest, opaque_to_host } to avoid repacking overhead
//   * passthrough/untyped address can be specified for function arguments/return values and for struct members
//   * opaque* can be specified for types, only, and is usually needed for C APIs using object-oriented style with "create_X"/"destroy_X"-like functions
// * For function parameters that are pointers to structs with a repacking scheme, use ptr_in/out/inout unless memory (de-)allocation is involved. The thunked API may only operate on the pointee data, since the pointers received by the native library will differ from the input pointers
//   * For const-pointers, use ptr_in only
//   * Factory functions that allocate the output pointer can't use this, as it's not clear how to deal with the object storage
//   * This can't be used for nested pointers
// * For other cases, manually implement the guest/host endpoints of the functions, e.g.:
//   * Factory functions

// Pointer doesn't need ABI fixups, since its value is not directly observed by the thunked library
struct ptr_pointer_passthrough {};

// Transitionary annotation for APIs that don't use semantic pointer annotations yet. Behaves like a passthrough pointer.
// DO NOT USE FOR NEW DEFINITIONS
struct [[deprecated]] ptr_todo_only64 {};

struct ptr_in {};
struct ptr_out {};
struct ptr_inout {};
struct ptr_is_untyped_address {};

struct opaque_to_guest {};
struct opaque_to_host {};

// Helper struct used to annotate function parameters via fex_gen_config,
// e.g. template<> struct fex_gen_config<fexgen::annotate_parameter<malloc, 0, void*>> {}
// Type is just for consistency checking, may be omitted by using 'void' for complicated types
template<auto F, int ParamIdx, typename Type>
struct annotate_param;

// Like annotate_parameter, but for the function return value
template<auto F, int ParamIdx, typename Type = decltype(nullptr)>
struct annotate_retval {};

// Used for padding members that need not be present on all architectures
struct is_padding_member {};

*/

} // namespace fexgen

// Takes either of
// * Function
// * Pointer-to-member
//   * ptr_pointer_passthrough
//   * ptr_is_untyped_address
//   * ptr_todo_only64 (DEPRECATED)
//template<auto>
//struct fex_gen_config;

// Takes a function, a parameter index, and the corresponding parameter type.
// The type is used for consistency checking, but may be omitted for complex types.
// Valid annotations:
// * ptr_in (trivial or repackable types, only)
// * ptr_out (trivial or repackable types, only)
// * ptr_inout (trivial or repackable types, only)
// * ptr_pointer_passthrough (not implemented)
// * ptr_is_untyped_address (not implemented)
// * ptr_todo_only64 (DEPRECATED) (not implemented)
template<auto, unsigned, typename = void>
struct fex_gen_param;

// Takes a function and the corresponding return value type.
// The type is used for consistency checking, but may be omitted for complex types.
template<auto, typename = void>
struct fex_gen_retval;

// Takes a type. Valid annotations are:
// * opaque_to_guest
// * opaque_to_host
template<typename>
struct fex_gen_type;
