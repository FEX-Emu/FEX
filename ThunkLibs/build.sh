GUEST_CXX=x86_64-linux-gnu-g++
HOST_CXX=g++

$GUEST_CXX -shared -fPIC libasound/libasound_Guest.cpp -o libasound-guest.so
$HOST_CXX -shared -fPIC libasound/libasound_Host.cpp -o libasound-host.so

$GUEST_CXX -shared -fPIC libSDL2/libSDL2_Guest.cpp -o libSDL2-guest.so
$HOST_CXX -shared -fPIC libSDL2/libSDL2_Host.cpp -o libSDL2-host.so

$GUEST_CXX -shared -fPIC libXext/libXext_Guest.cpp -o libXext-guest.so
$HOST_CXX -shared -fPIC libXext/libXext_Host.cpp -o libXext-host.so

$GUEST_CXX -shared -fPIC libXrender/libXrender_Guest.cpp -o libXrender-guest.so
$HOST_CXX -shared -fPIC libXrender/libXrender_Host.cpp -o libXrender-host.so

$GUEST_CXX -shared -fPIC libXfixes/libXfixes_Guest.cpp -o libXfixes-guest.so
$HOST_CXX -shared -fPIC libXfixes/libXfixes_Host.cpp -o libXfixes-host.so

$GUEST_CXX -shared -fPIC libEGL/libEGL_Guest.cpp -o libEGL-guest.so
$HOST_CXX -shared -fPIC libEGL/libEGL_Host.cpp -o libEGL-host.so

$GUEST_CXX -shared -fPIC libGL/libGL_Guest.cpp -o libGL-guest.so
$HOST_CXX -shared -fPIC libGL/libGL_Host.cpp -o libGL-host.so

$GUEST_CXX -shared -fPIC libX11/libX11_Guest.cpp -o libX11-guest.so
$HOST_CXX -shared -fPIC libX11/libX11_Host.cpp -o libX11-host.so
