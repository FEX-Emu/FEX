#pragma once

#define IMPL(Name) Name

extern "C" {
Display *IMPL(px11_AddGuestX11)(Display *Guest, const char *DisplayName);
void IMPL(px11_RemoveGuestX11)(Display *Guest);
Display *IMPL(px11_GuestToHostX11)(Display *Guest);
Display *IMPL(px11_HostToGuestX11)(Display *Host);

void IMPL(px11_FlushFromGuestX11)(Display *Guest);
void IMPL(px11_XFree)(void *p);
XVisualInfo *IMPL(px11_XVisual)(Display *Guest, int screen, unsigned int XVisual);
}

#undef IMPL