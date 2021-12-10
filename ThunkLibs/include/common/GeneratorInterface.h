namespace fexgen {
struct custom_host_impl {};

struct callback_annotation_base {
    // Prevent annotating multiple callback strategies
    bool prevent_multiple;
};
struct callback_stub : callback_annotation_base {};
struct callback_guest : callback_annotation_base {};

} // namespace fexgen
