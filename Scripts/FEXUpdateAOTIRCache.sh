#!/bin/sh
FEX=${1:-FEXLoader}
echo "Using $FEX"

for fileid in ~/.fex-emu/aotir/*.path; do
	filename=$(cat "$fileid")
	args=""

	# if L is 6 chars from the end, use localflags
	case $fileid in
		*L?????) _abi=--abilocalflags ;;
		*)    _abi=--no-abilocalflags ;;
	esac

	# if T is 7 chars from the end, use tso
	case $fileid in
		*T??????) _tso=--tsoenabled ;;
		*)     _tso=--no-tsoenabled ;;
	esac

	# if S is 8 chars from the end, use full smc
	case $fileid in
		*S???????) _smc=full ;;
		*)         _smc=mman ;;
	esac

	if [ -f "${fileid%.path}.aotir" ]; then
		echo "$(basename "$fileid") has already been generated"
	else
		echo "Processing $(basename "$fileid") ($filename) with $args"
		$FEX --aotirgenerate "$_abi" "$_tso" --smc="$_smc" "$filename"
	fi
done
