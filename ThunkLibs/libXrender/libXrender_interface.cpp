#include <common/GeneratorInterface.h>

#include <X11/extensions/Xrender.h>

#include <type_traits>

template<auto>
struct fex_gen_config {
    unsigned version = 1;
};

template<typename>
struct fex_gen_type {};

// Struct with multi-dimensional array member. Compatible data layout across all architectures
template<> struct fex_gen_type<_XTransform> : fexgen::assume_compatible_data_layout {};

#ifndef IS_32BIT_THUNK
// This has a public definition but is used as an opaque type in most APIs
template<> struct fex_gen_type<std::remove_pointer_t<Region>> : fexgen::assume_compatible_data_layout {};

// TODO: These are largely compatible, *but* contain function pointer members that need adjustment!
template<> struct fex_gen_type<XExtData> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<_XDisplay> : fexgen::assume_compatible_data_layout {};
#endif

template<> struct fex_gen_config<XRenderCreateAnimCursor> {};
template<> struct fex_gen_config<XRenderCreateCursor> {};
template<> struct fex_gen_config<XRenderCreateGlyphSet> {};
template<> struct fex_gen_config<XRenderReferenceGlyphSet> {};
template<> struct fex_gen_config<XRenderParseColor> {};
template<> struct fex_gen_config<XRenderQueryExtension> {};
template<> struct fex_gen_config<XRenderQueryFormats> {};
template<> struct fex_gen_config<XRenderQuerySubpixelOrder> {};
template<> struct fex_gen_config<XRenderQueryVersion> {};
template<> struct fex_gen_config<XRenderSetSubpixelOrder> {};
template<> struct fex_gen_config<XRenderCreateConicalGradient> {};
template<> struct fex_gen_config<XRenderCreateLinearGradient> {};
template<> struct fex_gen_config<XRenderCreatePicture> {};
template<> struct fex_gen_config<XRenderCreateRadialGradient> {};
template<> struct fex_gen_config<XRenderCreateSolidFill> {};
template<> struct fex_gen_config<XRenderAddGlyphs> {};
template<> struct fex_gen_config<XRenderAddTraps> {};
template<> struct fex_gen_config<XRenderChangePicture> {};
template<> struct fex_gen_config<XRenderComposite> {};
template<> struct fex_gen_config<XRenderCompositeDoublePoly> {};
template<> struct fex_gen_config<XRenderCompositeString16> {};
template<> struct fex_gen_config<XRenderCompositeString32> {};
template<> struct fex_gen_config<XRenderCompositeString8> {};
template<> struct fex_gen_config<XRenderCompositeText16> {};
template<> struct fex_gen_config<XRenderCompositeText32> {};
template<> struct fex_gen_config<XRenderCompositeText8> {};
template<> struct fex_gen_config<XRenderCompositeTrapezoids> {};
template<> struct fex_gen_config<XRenderCompositeTriangles> {};
template<> struct fex_gen_config<XRenderCompositeTriFan> {};
template<> struct fex_gen_config<XRenderCompositeTriStrip> {};
template<> struct fex_gen_config<XRenderFillRectangle> {};
template<> struct fex_gen_config<XRenderFillRectangles> {};
template<> struct fex_gen_config<XRenderFreeGlyphs> {};
template<> struct fex_gen_config<XRenderFreeGlyphSet> {};
template<> struct fex_gen_config<XRenderFreePicture> {};
template<> struct fex_gen_config<XRenderSetPictureClipRectangles> {};
template<> struct fex_gen_config<XRenderSetPictureClipRegion> {};
template<> struct fex_gen_config<XRenderSetPictureFilter> {};
template<> struct fex_gen_config<XRenderSetPictureTransform> {};
template<> struct fex_gen_config<XRenderQueryFilters> {};
template<> struct fex_gen_config<XRenderQueryPictIndexValues> {};
template<> struct fex_gen_config<XRenderFindFormat> {};
template<> struct fex_gen_config<XRenderFindStandardFormat> {};
template<> struct fex_gen_config<XRenderFindVisualFormat> {};
