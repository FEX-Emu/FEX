#!/bin/bash

# libGL - OpenGL + GLX

## guest side
python3 libGL-thunks.py thunks > ../ThunkLibs/libGL/libGL_Thunks.inl
python3 libGL-thunks.py symtab > ../ThunkLibs/libGL/libGL_Syms.inl #needed for glXGetProc

## host side
python3 libGL-thunks.py thunkmap     > ../ThunkLibs/libGL/libGL_Thunkmap.inl
python3 libGL-thunks.py initializers > ../ThunkLibs/libGL/libGL_Forwards.inl
python3 libGL-thunks.py forwards    >> ../ThunkLibs/libGL/libGL_Forwards.inl



# libX11

## guest side
python3 libX11-thunks.py thunks > ../ThunkLibs/libX11/libX11_Thunks.inl

## host side
python3 libX11-thunks.py thunkmap     > ../ThunkLibs/libX11/libX11_Thunkmap.inl
python3 libX11-thunks.py initializers > ../ThunkLibs/libX11/libX11_Forwards.inl
python3 libX11-thunks.py forwards    >> ../ThunkLibs/libX11/libX11_Forwards.inl



# libEGL

## guest side
python3 libEGL-thunks.py thunks > ../ThunkLibs/libEGL/libEGL_Thunks.inl

## host side
python3 libEGL-thunks.py thunkmap     > ../ThunkLibs/libEGL/libEGL_Thunkmap.inl
python3 libEGL-thunks.py initializers > ../ThunkLibs/libEGL/libEGL_Forwards.inl
python3 libEGL-thunks.py forwards    >> ../ThunkLibs/libEGL/libEGL_Forwards.inl