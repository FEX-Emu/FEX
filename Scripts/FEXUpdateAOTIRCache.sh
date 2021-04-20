#!/bin/bash
FEX={$1:FEXLoader}
for fileid in ~/.fex-emu/aotir/*.path; do
	filename=`cat "$fileid"`
	if [ -f "${fileid%.path}.aotir" ]; then
		echo "$filename has already been generated"
	else
		echo "Processing $filename ($fileid)"
		Bin/FEXLoader --aotirgenerate  "$filename"
	fi
done
