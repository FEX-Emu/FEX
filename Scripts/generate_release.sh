#!/bin/env bash

if [ "$#" -ne 2 ];
then
echo "$0: <NUMERIC-VERSION-PREV> <NUMERIC-VERSION-NEXT>"
exit -1
fi


echo "Tagging FEX-$2, previous release FEX-$1"
echo "Press cltr-c to cancel within 10 seconds"
sleep 10

git tag FEX-$2 -a -m "temporary"
Scripts/generate_doc_outline.sh > docs/SourceOutline.md
git commit docs/SourceOutline.md -m "Docs: Update for release FEX-$2"
git tad -d FEX-$2
git tag FEX-2104 -a -m "$(Scripts/generate_changelog.sh FEX-$1 FEX-$2)" --edit

echo "Inspect if everything went smooth via 'git log -6 FEX-$2' "
echo "if all is good, do 'git push FEX-$2:FEX-$2'"
