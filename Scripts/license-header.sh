#!/bin/sh -e

# SPDX-License-Identifier: MIT

# This script has been adapted from
# https://git.crueter.xyz/scripts/license-header/src/branch/master/.ci/license-header.sh

# Files to exclude (e.g. third-party files and scripts)
# You should specify these as a relative path to the root of the repository.
_exclude=""

# License identifier, get this from REUSE
_license="MIT"

_header="SPDX-License-Identifier: $_license"

# Contains files that need a license header.
BAD_FILES=""

BASE=$(git merge-base main HEAD)
FILES=$(git diff --name-only "$BASE")

for file in $FILES; do
	[ -f "$file" ] || continue

	# skip files that are third party
	excluded=false
	for pattern in $EXCLUDE_FILES; do
		case "$file" in
			*"$pattern"*) excluded=true; break ;;
		esac
	done

	[ "$excluded" = "false" ] || continue

	# Some file types just don't need headers.
	case "$file" in
		*.cmake | *.sh | *CMakeLists.txt | *.py | *.yml | *.cpp | *.c | *.h | *.qml) ;;
		*) continue ;;
	esac

	content="$(head -n5 <"$file")"

	if ! echo "$content" | grep -qe "$_header"; then
		BAD_FILES="$BAD_FILES $file"
	fi
done

if [ -z "$BAD_FILES" ]; then
	echo "-- All good."
	exit
else
	echo
	echo "-- The following files have incorrect license headers:"

	for file in $BAD_FILES; do echo "-- * $file"; done
	cat <<-EOF

	-- The following license header should be added to the start of these offending files as a comment:

	=== BEGIN ===
	$_header
	===  END  ===

    If some of the code in this PR is not being contributed by the original author,
    the files which have been exclusively changed by that code can be ignored.
    If this happens, this PR requirement can be bypassed once all other files are addressed.

	EOF
fi
