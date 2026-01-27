#!/bin/sh

# Allow release maintainer to override PREVIOUS and CURRENT by setting it before launching the script
PREVIOUS=${PREVIOUS:-FEX-$(date --date='-1 month' +%y%m)}
CURRENT=${CURRENT:-FEX-$(date +%y%m)}

if ! git rev-list "$PREVIOUS" > /dev/null 2>&1 ; then
  echo "$PREVIOUS tag doesn't exist"
  exit
fi

if git rev-list "$CURRENT" > /dev/null 2>&1 ; then
  echo "$CURRENT tag already exists!"
  exit
fi

echo "Tagging $CURRENT, previous release $PREVIOUS"
echo "Press Ctrl-C to cancel within 10 seconds"
sleep 10

git tag "$CURRENT" -a -m "temporary"
Scripts/generate_doc_outline.sh > docs/SourceOutline.md
git commit docs/SourceOutline.md -m "Docs: Update for release $CURRENT"
git tag -d "$CURRENT"
git tag -a "$CURRENT" -m "$(Scripts/generate_changelog.sh "$PREVIOUS" "$CURRENT")" --edit

echo "Inspect if everything went smoothly via 'git log -6 $CURRENT' "
echo "if all is good, do 'git push upstream $CURRENT'"
