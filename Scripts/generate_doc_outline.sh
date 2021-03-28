#!/bin/env bash
echo \# `git describe --always`
echo

./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/External/FEXCore" "../"
./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/ThunkLibs" "../"
./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/Scripts" "../"
./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/Source/Common" "../"
./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/Source/CommonCore" "../"
./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/Source/Tests" "../"
./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/unittests" "../"
