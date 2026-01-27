#!/bin/sh

echo "# $(git describe --always)"
echo

./Scripts/doc_outline_generator.py  "$(pwd)" "$(pwd)/FEXCore" "../"
./Scripts/doc_outline_generator.py  "$(pwd)" "$(pwd)/ThunkLibs" "../"
./Scripts/doc_outline_generator.py  "$(pwd)" "$(pwd)/Source/Tests" "../"
./Scripts/doc_outline_generator.py  "$(pwd)" "$(pwd)/unittests" "../"

# These don't have useful documentation at this point
#./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/Scripts" "../"
#./Scripts/doc_outline_generator.py  "`pwd`" "`pwd`/Source/Common" "../"
