#!/bin/env bash
if [ "$#" -ne 2 ];
then
echo "$0: <PEV-TAG> <NEXT-TAG>"
exit -1
fi
git log "$1..HEAD"  --pretty="%b (%h)" --abbrev-commit --merges | Scripts/changelog_generator.py "$2"
