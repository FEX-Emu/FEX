#!/bin/sh

for binfmt in "$@"; do
  result=0
  if command -v update-binfmts >/dev/null; then
	# If we found the binfmt_misc file passed in then error
    update-binfmts --find "$binfmt" 1>&- 2>&- && result=1
  fi

  # If the binfmt_misc file exists then error
  [ -f "$binfmt" ] && result=1

  if [ $result -eq 1 ]; then
    echo "==============================================================="
    echo "$binfmt binfmt file is installed!"
    echo "This conflicts with FEX-Emu's binfmt_misc!"
    echo "This will cause issues when running FEX-Emu through binfmt_misc"
    echo "Not installing until you uninstall this binfmt_misc file!"
    echo "==============================================================="
    exit 1
  fi
done

exit 0
