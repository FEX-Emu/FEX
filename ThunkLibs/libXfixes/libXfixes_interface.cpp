#include <common/GeneratorInterface.h>

#include <X11/extensions/Xfixes.h>

template<auto>
struct fex_gen_config {
    unsigned version = 3;
};

template<> struct fex_gen_type<Display> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<_XGC> : fexgen::opaque_to_guest {};

template<> struct fex_gen_config<XFixesGetCursorName> {};
template<> struct fex_gen_config<XFixesQueryExtension> {};
template<> struct fex_gen_config<XFixesQueryVersion> {};
template<> struct fex_gen_config<XFixesVersion> {};
template<> struct fex_gen_config<XFixesCreatePointerBarrier> {};
template<> struct fex_gen_config<XFixesChangeCursorByName> {};
template<> struct fex_gen_config<XFixesChangeCursor> {};
template<> struct fex_gen_config<XFixesChangeSaveSet> {};
template<> struct fex_gen_config<XFixesCopyRegion> {};
template<> struct fex_gen_config<XFixesDestroyPointerBarrier> {};
template<> struct fex_gen_config<XFixesDestroyRegion> {};
template<> struct fex_gen_config<XFixesExpandRegion> {};
template<> struct fex_gen_config<XFixesHideCursor> {};
template<> struct fex_gen_config<XFixesIntersectRegion> {};
template<> struct fex_gen_config<XFixesInvertRegion> {};
template<> struct fex_gen_config<XFixesRegionExtents> {};
template<> struct fex_gen_config<XFixesSelectCursorInput> {};
template<> struct fex_gen_config<XFixesSelectSelectionInput> {};
template<> struct fex_gen_config<XFixesSetCursorName> {};
template<> struct fex_gen_config<XFixesSetGCClipRegion> {};
template<> struct fex_gen_config<XFixesSetPictureClipRegion> {};
template<> struct fex_gen_config<XFixesSetRegion> {};
template<> struct fex_gen_config<XFixesSetWindowShapeRegion> {};
template<> struct fex_gen_config<XFixesShowCursor> {};
template<> struct fex_gen_config<XFixesSubtractRegion> {};
template<> struct fex_gen_config<XFixesTranslateRegion> {};
template<> struct fex_gen_config<XFixesUnionRegion> {};
template<> struct fex_gen_config<XFixesGetCursorImage> {};
template<> struct fex_gen_config<XFixesFetchRegionAndBounds> {};
template<> struct fex_gen_config<XFixesFetchRegion> {};
template<> struct fex_gen_config<XFixesCreateRegion> {};
template<> struct fex_gen_config<XFixesCreateRegionFromBitmap> {};
template<> struct fex_gen_config<XFixesCreateRegionFromGC> {};
template<> struct fex_gen_config<XFixesCreateRegionFromPicture> {};
template<> struct fex_gen_config<XFixesCreateRegionFromWindow> {};
