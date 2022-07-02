#!/bin/sh
for binfmt in "$@"
do
  result=0
  if command -v update-binfmts>/dev/null; then
    update-binfmts --find "$binfmt" 1>&- 2>&-
    if [ $? -eq 0 ]
    then
      # If we found the binfmt_misc file passed in then error
      result=1
    fi
  fi

  if [ $result -eq 0 ]
  then
    if [ -f "$binfmt" ]; then
      # If the binfmt_misc file exists then error
      result=1
    fi
  fi

  if [ $result -eq 1 ]
  then
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
