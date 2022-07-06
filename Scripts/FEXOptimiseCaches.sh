#!/bin/bash
FEXAOTGen=${1:-FEXAOTGen}
echo Using $FEXAOTGen
for fileid in ~/.fex-emu/JitCache/*/Path; do
	filename=`cat "$fileid"`
	args=""
	if [ "${fileid: -6 : 1}" == "P" ]; then
		args="$args --no-abinopf"
	else
		args="$args --abinopf"
	fi

	if [ "${fileid: -7 : 1}" == "L" ]; then
		args="$args --abilocalflags"
	else
		args="$args --no-abilocalflags"
	fi
	
	if [ "${fileid: -8 : 1}" == "T" ]; then
		args="$args --tsoenabled"
	else
		args="$args --no-tsoenabled"
	fi
	
	if [ "${fileid: -9 : 1}" == "S" ]; then
		args="$args --smc=full"
	else
		args="$args --smc=mtrack"
	fi

	echo "Processing `basename $fileid` ($filename) with $args"
	$FEXAOTGen $args "$filename"
done
