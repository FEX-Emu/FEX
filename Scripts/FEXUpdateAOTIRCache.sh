#!/bin/bash
FEX=${1:-FEXLoader}
echo Using $FEX
for fileid in ~/.fex-emu/aotir/*.path; do
	filename=`cat "$fileid"`
	args=""
	if [ "${fileid: -6 : 1}" == "L" ]; then
		args="$args --abilocalflags"
	else
		args="$args --no-abilocalflags"
	fi

	if [ "${fileid: -7 : 1}" == "T" ]; then
		args="$args --tsoenabled"
	else
		args="$args --no-tsoenabled"
	fi

	if [ "${fileid: -8 : 1}" == "S" ]; then
		args="$args --smc=full"
	else
		args="$args --smc=mman"
	fi

	if [ -f "${fileid%.path}.aotir" ]; then
		echo "`basename $fileid` has already been generated"
	else
		echo "Processing `basename $fileid` ($filename) with $args"
		$FEX --aotirgenerate $args "$filename"
	fi
done
