from enum import Flag
import json
import struct
import sys
from json_config_parse import parse_json

if (len(sys.argv) < 3):
    sys.exit()

output_file = sys.argv[2]
asm_file = open(sys.argv[1], "r")
asm_text = asm_file.read()
asm_file.close()

json_text = asm_text.split(";%ifdef CONFIG")
if (len(json_text) > 1):
        json_text = json_text[1].split(";%endif")
        if (len(json_text) > 1):
            json_text = json_text[0].strip()
            # We need to walk each line of text and remove the comment line
            json_text = json_text.splitlines(False)
            parsed_lines = ""
            for line in json_text:
                line = line.strip()
                if (line[0] != ';'):
                    sys.exit("Config line needs to start with a comment character ;")
                line = line.lstrip(";")

                parsed_lines = parsed_lines + line + '\n'

            parsed_lines = parsed_lines.strip()
            parse_json(parsed_lines, output_file)
