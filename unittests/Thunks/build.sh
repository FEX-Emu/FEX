#!/bin/bash

g++ thunk-test.cpp  -o ../../build/thunk-test.elf

python3 generate-thunk.py thunks > glx-thunks.inl && g++ glx-thunks.cpp gl3example.cpp -o ../../build/gl3thunked.elf

python3 generate-thunk.py thunkmap > ../../External/FEXCore/Source/Interface/HLE/Thunks/GLX_thunkmap.inl
python3 generate-thunk.py initializers > ../../External/FEXCore/Source/Interface/HLE/Thunks/GLX_forwards.inl
python3 generate-thunk.py forwards >> ../../External/FEXCore/Source/Interface/HLE/Thunks/GLX_forwards.inl

