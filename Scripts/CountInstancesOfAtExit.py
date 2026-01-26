#!/usr/bin/python3
import sys
import subprocess

def main():
    # Args: <FEX bin> <string> <count>
    if (len(sys.argv) < 4):
        sys.exit()

    result = subprocess.run(['sh', '-c', "llvm-objdump -D {} | grep \'{}\' | wc -l".format(sys.argv[1], sys.argv[2])], stdout=subprocess.PIPE)
    Count = int(result.stdout.decode('ascii'))

    if (Count != int(sys.argv[3])):
        sys.exit(-1)

    return 0

if __name__ == "__main__":
    # execute only if run as a script
    sys.exit(main())

