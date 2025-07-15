#! /bin/bash

# Save current directory
DIR=$(pwd)

# Change to the directory passed as argument if any
if [ $# -eq 1 ]; then
    cd $1
fi

# Reformat whole tree.
# This is run by the reformat target.
git ls-files -z '*.cpp' '*.h' '*.inl' | xargs -0 -n 1 -P $(nproc) python3 Scripts/clang-format.py -i
cd $DIR
