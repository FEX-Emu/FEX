/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <stdio.h>

extern "C" {
#define XUTIL_DEFINE_FUNCTIONS
#include <X11/Xproto.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#undef min
#undef max

#define XTRANS_SEND_FDS 1
#include <X11/Xtrans/Xtransint.h>

#include <X11/Xutil.h>
#include <X11/Xregion.h>
#include <X11/Xresource.h>

#include <X11/Xproto.h>

#include <X11/extensions/extutil.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBgeom.h>

#include <X11/Xtrans/Xtransint.h>
}

#include "common/Host.h"
#include <dlfcn.h>
#include <utility>

#include "thunkgen_host_libX11.inl"
#include "X11Common.h"

// Walks the array of key:value pairs provided from the guest code side.
// Finalizing any callback functions that the guest side prepared for us.
template<typename CallbackType>
void FinalizeIncomingCallbacks(size_t Count, void **list) {
  assert(Count % 2 == 0 && "Incoming arguments needs to be in pairs");
  for (size_t i = 0; i < (Count / 2); ++i) {
    const char *Key = static_cast<const char*>(list[i * 2]);
    void** Data = &list[i * 2 + 1];
    if (!*Data) {
      continue;
    }

    // Check if the key is a callback and needs to be modified.
    auto KeyIt = std::find(X11::CallbackKeys.begin(), X11::CallbackKeys.end(), Key);
    if (KeyIt == X11::CallbackKeys.end()) {
      continue;
    }

    CallbackType *IncomingCallback = reinterpret_cast<CallbackType*>(*Data);
    FinalizeHostTrampolineForGuestFunction(IncomingCallback->callback);
  }
}

_XIC *fexfn_impl_libX11_XCreateIC_internal(XIM a_0, size_t count, void **list) {
  FinalizeIncomingCallbacks<XICCallback>(count, list);

  switch(count) {
    case 0: return fexldr_ptr_libX11_XCreateIC(a_0, nullptr);
      break;
    case 1: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], nullptr);
      break;
    case 2: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], nullptr);
      break;
    case 3: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], nullptr);
      break;
    case 4: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], nullptr);
      break;
    case 5: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], nullptr);
      break;
    case 6: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr);
      break;
    case 7: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr);
      break;
    case 8: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], nullptr);
      break;
    case 9: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], nullptr);
      break;
    case 10: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], nullptr);
      break;
    case 11: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], nullptr);
      break;
    case 12: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], nullptr);
      break;
    case 13: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], nullptr);
      break;
    case 14: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], nullptr);
      break;
    case 15: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], nullptr);
      break;
    case 16: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15], nullptr);
      break;
    case 17: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], nullptr);
      break;
    case 18: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], nullptr);
      break;
    case 19: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], nullptr);
      break;
    case 20: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], nullptr);
      break;
    case 21: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], nullptr);
      break;
    case 22: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], nullptr);
      break;
    case 23: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], nullptr);
      break;
    case 24: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23], nullptr);
      break;
    case 25: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], nullptr);
      break;
    case 26: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], nullptr);
      break;
    case 27: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], nullptr);
      break;
    case 28: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], nullptr);
      break;
    case 29: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], nullptr);
      break;
    case 30: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], nullptr);
      break;
    case 31: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], nullptr);
      break;
    case 32: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31], nullptr);
      break;
    case 33: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], nullptr);
      break;
    case 34: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], nullptr);
      break;
    case 35: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], nullptr);
      break;
    case 36: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], nullptr);
      break;
    case 37: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], nullptr);
      break;
    case 38: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], nullptr);
      break;
    case 39: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], nullptr);
      break;
    case 40: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39], nullptr);
      break;
    case 41: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], nullptr);
      break;
    case 42: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], nullptr);
      break;
    case 43: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], nullptr);
      break;
    case 44: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], nullptr);
      break;
    case 45: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], nullptr);
      break;
    case 46: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], nullptr);
      break;
    case 47: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], nullptr);
      break;
    case 48: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47], nullptr);
      break;
    case 49: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], nullptr);
      break;
    case 50: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], nullptr);
      break;
    case 51: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], nullptr);
      break;
    case 52: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], nullptr);
      break;
    case 53: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], nullptr);
      break;
    case 54: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], nullptr);
      break;
    case 55: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], nullptr);
      break;
    case 56: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55], nullptr);
      break;
    case 57: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], nullptr);
      break;
    case 58: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], nullptr);
      break;
    case 59: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], nullptr);
      break;
    case 60: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], nullptr);
      break;
    case 61: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], nullptr);
      break;
    case 62: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], nullptr);
      break;
    case 63: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], list[62], nullptr);
      break;
    default:
      fprintf(stderr, "XCreateIC_internal FAILURE\n");
      return nullptr;
  }
}

char* fexfn_impl_libX11_XGetICValues_internal(XIC a_0, size_t count, void **list) {
  switch(count) {
    case 0: return fexldr_ptr_libX11_XGetICValues(a_0, nullptr);
      break;
    case 1: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], nullptr);
      break;
    case 2: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], nullptr);
      break;
    case 3: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], nullptr);
      break;
    case 4: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], nullptr);
      break;
    case 5: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr);
      break;
    case 6: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr);
      break;
    case 7: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr);
      break;
    case 8: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], nullptr);
      break;
    case 9: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], nullptr);
      break;
    case 10: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], nullptr);
      break;
    case 11: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], nullptr);
      break;
    case 12: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], nullptr);
      break;
    case 13: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], nullptr);
      break;
    case 14: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], nullptr);
      break;
    case 15: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], nullptr);
      break;
    case 16: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15], nullptr);
      break;
    case 17: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], nullptr);
      break;
    case 18: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], nullptr);
      break;
    case 19: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], nullptr);
      break;
    case 20: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], nullptr);
      break;
    case 21: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], nullptr);
      break;
    case 22: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], nullptr);
      break;
    case 23: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], nullptr);
      break;
    case 24: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23], nullptr);
      break;
    case 25: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], nullptr);
      break;
    case 26: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], nullptr);
      break;
    case 27: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], nullptr);
      break;
    case 28: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], nullptr);
      break;
    case 29: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], nullptr);
      break;
    case 30: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], nullptr);
      break;
    case 31: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], nullptr);
      break;
    case 32: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31], nullptr);
      break;
    case 33: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], nullptr);
      break;
    case 34: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], nullptr);
      break;
    case 35: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], nullptr);
      break;
    case 36: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], nullptr);
      break;
    case 37: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], nullptr);
      break;
    case 38: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], nullptr);
      break;
    case 39: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], nullptr);
      break;
    case 40: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39], nullptr);
      break;
    case 41: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], nullptr);
      break;
    case 42: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], nullptr);
      break;
    case 43: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], nullptr);
      break;
    case 44: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], nullptr);
      break;
    case 45: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], nullptr);
      break;
    case 46: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], nullptr);
      break;
    case 47: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], nullptr);
      break;
    case 48: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47], nullptr);
      break;
    case 49: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], nullptr);
      break;
    case 50: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], nullptr);
      break;
    case 51: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], nullptr);
      break;
    case 52: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], nullptr);
      break;
    case 53: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], nullptr);
      break;
    case 54: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], nullptr);
      break;
    case 55: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], nullptr);
      break;
    case 56: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55], nullptr);
      break;
    case 57: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], nullptr);
      break;
    case 58: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], nullptr);
      break;
    case 59: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], nullptr);
      break;
    case 60: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], nullptr);
      break;
    case 61: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], nullptr);
      break;
    case 62: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], nullptr);
      break;
    case 63: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], list[62], nullptr);
      break;
    default:
      fprintf(stderr, "XGetICValues_internal FAILURE\n");
      abort();
  }
}

char* fexfn_impl_libX11_XSetICValues_internal(XIC a_0, size_t count, void **list) {
  FinalizeIncomingCallbacks<XICCallback>(count, list);

  switch(count) {
    case 0: return fexldr_ptr_libX11_XSetICValues(a_0, nullptr);
      break;
    case 1: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], nullptr);
      break;
    case 2: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], nullptr);
      break;
    case 3: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], nullptr);
      break;
    case 4: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], nullptr);
      break;
    case 5: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr);
      break;
    case 6: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr);
      break;
    case 7: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr);
      break;
    case 8: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], nullptr);
      break;
    case 9: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], nullptr);
      break;
    case 10: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], nullptr);
      break;
    case 11: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], nullptr);
      break;
    case 12: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], nullptr);
      break;
    case 13: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], nullptr);
      break;
    case 14: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], nullptr);
      break;
    case 15: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], nullptr);
      break;
    case 16: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15], nullptr);
      break;
    case 17: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], nullptr);
      break;
    case 18: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], nullptr);
      break;
    case 19: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], nullptr);
      break;
    case 20: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], nullptr);
      break;
    case 21: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], nullptr);
      break;
    case 22: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], nullptr);
      break;
    case 23: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], nullptr);
      break;
    case 24: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23], nullptr);
      break;
    case 25: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], nullptr);
      break;
    case 26: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], nullptr);
      break;
    case 27: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], nullptr);
      break;
    case 28: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], nullptr);
      break;
    case 29: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], nullptr);
      break;
    case 30: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], nullptr);
      break;
    case 31: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], nullptr);
      break;
    case 32: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31], nullptr);
      break;
    case 33: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], nullptr);
      break;
    case 34: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], nullptr);
      break;
    case 35: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], nullptr);
      break;
    case 36: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], nullptr);
      break;
    case 37: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], nullptr);
      break;
    case 38: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], nullptr);
      break;
    case 39: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], nullptr);
      break;
    case 40: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39], nullptr);
      break;
    case 41: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], nullptr);
      break;
    case 42: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], nullptr);
      break;
    case 43: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], nullptr);
      break;
    case 44: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], nullptr);
      break;
    case 45: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], nullptr);
      break;
    case 46: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], nullptr);
      break;
    case 47: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], nullptr);
      break;
    case 48: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47], nullptr);
      break;
    case 49: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], nullptr);
      break;
    case 50: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], nullptr);
      break;
    case 51: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], nullptr);
      break;
    case 52: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], nullptr);
      break;
    case 53: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], nullptr);
      break;
    case 54: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], nullptr);
      break;
    case 55: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], nullptr);
      break;
    case 56: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55], nullptr);
      break;
    case 57: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], nullptr);
      break;
    case 58: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], nullptr);
      break;
    case 59: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], nullptr);
      break;
    case 60: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], nullptr);
      break;
    case 61: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], nullptr);
      break;
    case 62: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], nullptr);
      break;
    case 63: return fexldr_ptr_libX11_XSetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], list[62], nullptr);
      break;
    default:
      fprintf(stderr, "XSetICValues_internal FAILURE\n");
      abort();
  }
}

char* fexfn_impl_libX11_XGetIMValues_internal(XIM a_0, size_t count, void **list) {
  switch(count) {
    case 0: return fexldr_ptr_libX11_XGetIMValues(a_0, nullptr);
      break;
    case 1: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], nullptr);
      break;
    case 2: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], nullptr);
      break;
    case 3: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], nullptr);
      break;
    case 4: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], nullptr);
      break;
    case 5: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr);
      break;
    case 6: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr);
      break;
    case 7: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr);
      break;
    case 8: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], nullptr);
      break;
    case 9: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], nullptr);
      break;
    case 10: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], nullptr);
      break;
    case 11: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], nullptr);
      break;
    case 12: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], nullptr);
      break;
    case 13: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], nullptr);
      break;
    case 14: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], nullptr);
      break;
    case 15: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], nullptr);
      break;
    case 16: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15], nullptr);
      break;
    case 17: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], nullptr);
      break;
    case 18: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], nullptr);
      break;
    case 19: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], nullptr);
      break;
    case 20: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], nullptr);
      break;
    case 21: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], nullptr);
      break;
    case 22: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], nullptr);
      break;
    case 23: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], nullptr);
      break;
    case 24: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23], nullptr);
      break;
    case 25: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], nullptr);
      break;
    case 26: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], nullptr);
      break;
    case 27: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], nullptr);
      break;
    case 28: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], nullptr);
      break;
    case 29: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], nullptr);
      break;
    case 30: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], nullptr);
      break;
    case 31: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], nullptr);
      break;
    case 32: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31], nullptr);
      break;
    case 33: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], nullptr);
      break;
    case 34: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], nullptr);
      break;
    case 35: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], nullptr);
      break;
    case 36: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], nullptr);
      break;
    case 37: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], nullptr);
      break;
    case 38: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], nullptr);
      break;
    case 39: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], nullptr);
      break;
    case 40: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39], nullptr);
      break;
    case 41: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], nullptr);
      break;
    case 42: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], nullptr);
      break;
    case 43: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], nullptr);
      break;
    case 44: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], nullptr);
      break;
    case 45: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], nullptr);
      break;
    case 46: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], nullptr);
      break;
    case 47: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], nullptr);
      break;
    case 48: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47], nullptr);
      break;
    case 49: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], nullptr);
      break;
    case 50: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], nullptr);
      break;
    case 51: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], nullptr);
      break;
    case 52: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], nullptr);
      break;
    case 53: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], nullptr);
      break;
    case 54: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], nullptr);
      break;
    case 55: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], nullptr);
      break;
    case 56: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55], nullptr);
      break;
    case 57: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], nullptr);
      break;
    case 58: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], nullptr);
      break;
    case 59: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], nullptr);
      break;
    case 60: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], nullptr);
      break;
    case 61: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], nullptr);
      break;
    case 62: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], nullptr);
      break;
    case 63: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], list[62], nullptr);
      break;
    default:
      fprintf(stderr, "XGetIMValues_internal FAILURE\n");
      abort();
  }
}

char* fexfn_impl_libX11_XSetIMValues_internal(XIM a_0, size_t count, void **list) {
  FinalizeIncomingCallbacks<XIMCallback>(count, list);

  switch(count) {
    case 0: return fexldr_ptr_libX11_XSetIMValues(a_0, nullptr);
      break;
    case 1: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], nullptr);
      break;
    case 2: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], nullptr);
      break;
    case 3: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], nullptr);
      break;
    case 4: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], nullptr);
      break;
    case 5: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr);
      break;
    case 6: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr);
      break;
    case 7: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr);
      break;
    case 8: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], nullptr);
      break;
    case 9: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], nullptr);
      break;
    case 10: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], nullptr);
      break;
    case 11: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], nullptr);
      break;
    case 12: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], nullptr);
      break;
    case 13: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], nullptr);
      break;
    case 14: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], nullptr);
      break;
    case 15: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], nullptr);
      break;
    case 16: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15], nullptr);
      break;
    case 17: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], nullptr);
      break;
    case 18: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], nullptr);
      break;
    case 19: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], nullptr);
      break;
    case 20: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], nullptr);
      break;
    case 21: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], nullptr);
      break;
    case 22: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], nullptr);
      break;
    case 23: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], nullptr);
      break;
    case 24: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23], nullptr);
      break;
    case 25: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], nullptr);
      break;
    case 26: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], nullptr);
      break;
    case 27: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], nullptr);
      break;
    case 28: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], nullptr);
      break;
    case 29: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], nullptr);
      break;
    case 30: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], nullptr);
      break;
    case 31: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], nullptr);
      break;
    case 32: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31], nullptr);
      break;
    case 33: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], nullptr);
      break;
    case 34: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], nullptr);
      break;
    case 35: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], nullptr);
      break;
    case 36: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], nullptr);
      break;
    case 37: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], nullptr);
      break;
    case 38: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], nullptr);
      break;
    case 39: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], nullptr);
      break;
    case 40: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39], nullptr);
      break;
    case 41: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], nullptr);
      break;
    case 42: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], nullptr);
      break;
    case 43: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], nullptr);
      break;
    case 44: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], nullptr);
      break;
    case 45: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], nullptr);
      break;
    case 46: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], nullptr);
      break;
    case 47: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], nullptr);
      break;
    case 48: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47], nullptr);
      break;
    case 49: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], nullptr);
      break;
    case 50: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], nullptr);
      break;
    case 51: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], nullptr);
      break;
    case 52: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], nullptr);
      break;
    case 53: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], nullptr);
      break;
    case 54: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], nullptr);
      break;
    case 55: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], nullptr);
      break;
    case 56: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55], nullptr);
      break;
    case 57: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], nullptr);
      break;
    case 58: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], nullptr);
      break;
    case 59: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], nullptr);
      break;
    case 60: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], nullptr);
      break;
    case 61: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], nullptr);
      break;
    case 62: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], nullptr);
      break;
    case 63: return fexldr_ptr_libX11_XSetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], list[62], nullptr);
      break;
    default:
      fprintf(stderr, "XSetIMValues_internal FAILURE\n");
      abort();
  }
}

XVaNestedList fexfn_impl_libX11_XVaCreateNestedList_internal(int unused_arg, size_t count, void **list) {
  FinalizeIncomingCallbacks<XIMCallback>(count, list);

  switch(count) {
    case 0: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, nullptr);
      break;
    case 1: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], nullptr);
      break;
    case 2: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], nullptr);
      break;
    case 3: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], nullptr);
      break;
    case 4: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], nullptr);
      break;
    case 5: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], nullptr);
      break;
    case 6: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], nullptr);
      break;
    case 7: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr);
      break;
    case 8: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], nullptr);
      break;
    case 9: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], nullptr);
      break;
    case 10: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], nullptr);
      break;
    case 11: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], nullptr);
      break;
    case 12: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], nullptr);
      break;
    case 13: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], nullptr);
      break;
    case 14: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], nullptr);
      break;
    case 15: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], nullptr);
      break;
    case 16: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15], nullptr);
      break;
    case 17: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], nullptr);
      break;
    case 18: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], nullptr);
      break;
    case 19: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], nullptr);
      break;
    case 20: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], nullptr);
      break;
    case 21: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], nullptr);
      break;
    case 22: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], nullptr);
      break;
    case 23: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], nullptr);
      break;
    case 24: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23], nullptr);
      break;
    case 25: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], nullptr);
      break;
    case 26: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], nullptr);
      break;
    case 27: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], nullptr);
      break;
    case 28: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], nullptr);
      break;
    case 29: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], nullptr);
      break;
    case 30: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], nullptr);
      break;
    case 31: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], nullptr);
      break;
    case 32: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31], nullptr);
      break;
    case 33: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], nullptr);
      break;
    case 34: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], nullptr);
      break;
    case 35: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], nullptr);
      break;
    case 36: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], nullptr);
      break;
    case 37: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], nullptr);
      break;
    case 38: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], nullptr);
      break;
    case 39: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], nullptr);
      break;
    case 40: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39], nullptr);
      break;
    case 41: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], nullptr);
      break;
    case 42: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], nullptr);
      break;
    case 43: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], nullptr);
      break;
    case 44: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], nullptr);
      break;
    case 45: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], nullptr);
      break;
    case 46: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], nullptr);
      break;
    case 47: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], nullptr);
      break;
    case 48: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47], nullptr);
      break;
    case 49: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], nullptr);
      break;
    case 50: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], nullptr);
      break;
    case 51: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], nullptr);
      break;
    case 52: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], nullptr);
      break;
    case 53: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], nullptr);
      break;
    case 54: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], nullptr);
      break;
    case 55: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], nullptr);
      break;
    case 56: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55], nullptr);
      break;
    case 57: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], nullptr);
      break;
    case 58: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], nullptr);
      break;
    case 59: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], nullptr);
      break;
    case 60: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], nullptr);
      break;
    case 61: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], nullptr);
      break;
    case 62: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], nullptr);
      break;
    case 63: return fexldr_ptr_libX11_XVaCreateNestedList(unused_arg, list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7],
      list[8], list[9], list[10], list[11], list[12], list[13], list[14], list[15],
      list[16], list[17], list[18], list[19], list[20], list[21], list[22], list[23],
      list[24], list[25], list[26], list[27], list[28], list[29], list[30], list[31],
      list[32], list[33], list[34], list[35], list[36], list[37], list[38], list[39],
      list[40], list[41], list[42], list[43], list[44], list[45], list[46], list[47],
      list[48], list[49], list[50], list[51], list[52], list[53], list[54], list[55],
      list[56], list[57], list[58], list[59], list[60], list[61], list[62], nullptr);
      break;
    default:
      fprintf(stderr, "XVaCreateNestedList_internal FAILURE\n");
      abort();
  }
}

Status fexfn_impl_libX11_XInitThreadsInternal(uintptr_t, uintptr_t);

Status fexfn_impl_libX11__XReply(Display*, xReply*, int, Bool);

static int (*ACTUAL_XInitDisplayLock_fn)(Display*) = nullptr;
static int (*INTERNAL_XInitDisplayLock_fn)(Display*) = nullptr;

static std::atomic<bool> Initialized{};

static int _XInitDisplayLock(Display* display) {
  auto ret = ACTUAL_XInitDisplayLock_fn(display);
  INTERNAL_XInitDisplayLock_fn(display);
  return ret;
}

Status fexfn_impl_libX11_XInitThreadsInternal(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  bool Expected = false;
  if (!Initialized.compare_exchange_strong(Expected, true)) {
    // If already initialized then this is a no-op.
    return 1;
  }
  auto ret = fexldr_ptr_libX11_XInitThreads();
  auto _XInitDisplayLock_fn = (int(**)(Display*))dlsym(fexldr_ptr_libX11_so, "_XInitDisplayLock_fn");
  ACTUAL_XInitDisplayLock_fn = std::exchange(*_XInitDisplayLock_fn, _XInitDisplayLock);
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &INTERNAL_XInitDisplayLock_fn);
  return ret;
}

Status fexfn_impl_libX11__XReply(Display* display, xReply* reply, int extra, Bool discard) {
  for(auto handler = display->async_handlers; handler; handler = handler->next) {
    FinalizeHostTrampolineForGuestFunction(handler->handler);
  }
  return fexldr_ptr_libX11__XReply(display, reply, extra, discard);
}

EXPORTS(libX11)
