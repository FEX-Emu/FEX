g++ -shared -fPIC libXext/libXext_Guest.cpp -o libXext-guest.so
g++ -shared -fPIC libXext/libXext_Host.cpp -o libXext-host.so

g++ -shared -fPIC libXrender/libXrender_Guest.cpp -o libXrender-guest.so
g++ -shared -fPIC libXrender/libXrender_Host.cpp -o libXrender-host.so

g++ -shared -fPIC libXfixes/libXfixes_Guest.cpp -o libXfixes-guest.so
g++ -shared -fPIC libXfixes/libXfixes_Host.cpp -o libXfixes-host.so

g++ -shared -fPIC libEGL/libEGL_Guest.cpp -o libEGL-guest.so
g++ -shared -fPIC libEGL/libEGL_Host.cpp -o libEGL-host.so

g++ -shared -fPIC libGL/libGL_Guest.cpp -o libGL-guest.so
g++ -shared -fPIC libGL/libGL_Host.cpp -o libGL-host.so

g++ -shared -fPIC libX11/libX11_Guest.cpp -o libX11-guest.so
g++ -shared -fPIC libX11/libX11_Host.cpp -o libX11-host.so
