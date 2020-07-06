#!/bin/bash

g++ thunk-test.cpp  -o ../../build/thunk-test.elf

python3 generate-thunk.py thunks > glx-thunks.inl
python3 generate-thunk.py symtab > glx-syms.inl
g++ glx-thunks.cpp gl3example.cpp -o ../../build/gl3thunked.elf

python3 generate-thunk.py thunkmap > ../../External/FEXCore/Source/Interface/HLE/Thunks/GLX_thunkmap.inl
python3 generate-thunk.py initializers > ../../External/FEXCore/Source/Interface/HLE/Thunks/GLX_forwards.inl
python3 generate-thunk.py forwards >> ../../External/FEXCore/Source/Interface/HLE/Thunks/GLX_forwards.inl

#./build.sh  2>&1 | grep previous | tr '’‘' '|' | cut -f 4 -d '|'
g++ -shared -fPIC glx-thunks.cpp -o libThunk.so
mkdir -p ../../build/rootfs/lib/x86_64-linux-gnu/
mv -f libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libX11.so.6
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libGLdispatch.so.0
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libGL.so.1
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libGLX.so.0
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libxcb.so.1
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libXau.so.6
ln -s ../../build/rootfs/lib/x86_64-linux-gnu/libThunk.so ../../build/rootfs/lib/x86_64-linux-gnu/libXdmcp.so.6
