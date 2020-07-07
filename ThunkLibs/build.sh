g++ -shared -fPIC libEGL/libEGL_Guest.cpp -o libEGL-guest.so
g++ -shared -fPIC libEGL/libEGL_Host.cpp -o libEGL-host.so

g++ -shared -fPIC libGL/libGL_Guest.cpp -o libGL-guest.so
g++ -shared -fPIC libGL/libGL_Host.cpp -o libGL-host.so

g++ -shared -fPIC libX11/libX11_Guest.cpp -o libX11-guest.so
g++ -shared -fPIC libX11/libX11_Host.cpp -o libX11-host.so
