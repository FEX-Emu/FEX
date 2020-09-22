#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xext.h>
//#include <X11/extensions/extutil.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/agproto.h>
#include <X11/extensions/ag.h>
#include <X11/extensions/cup.h>
#include <X11/extensions/dbe.h>
#include <X11/extensions/dbeproto.h>
#include <X11/extensions/XEVI.h>
#include <X11/extensions/Xge.h>
#include <X11/extensions/XLbx.h>
#include <X11/extensions/multibuf.h>
#include <X11/extensions/MITMisc.h>
#include <X11/extensions/mitmiscconst.h>
#include <X11/extensions/mitmiscproto.h>
#include <X11/extensions/security.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/shapeconst.h>
#include <X11/extensions/shapeproto.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/sync.h>
#include <X11/extensions/syncconst.h>
#include <X11/extensions/syncproto.h>
//#include <X11/extensions/XTest.h>


#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

LOAD_LIB(libXext)
