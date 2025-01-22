#!/bin/env python3
import sys

import fileinput
import re

# Handles the following formats:
#
# <commit message> -> goes in Misc category
# <Category>: <commit message> -> Goes in <Category>
# <Category>/<Tag>: <commit message> -> Goes in <Category>/<Tag>

Meta = { }
for line in sys.stdin.readlines():
    if detailed := re.findall("^([A-Za-z0-9]+)/([A-Za-z0-9]+):(.+)$", line):
        detailed = detailed[0]
        meta = detailed[0].strip() + "/" + detailed[1].strip()
        if meta not in Meta:
            Meta[meta] = []
        Meta[meta].append(detailed[2].strip())
    elif category := re.findall("(^[A-Za-z0-9]+):(.+)$", line):
        category = category[0]
        if category[0].strip() not in Meta:
            Meta[category[0].strip()] = []
        Meta[category[0].strip()].append(category[1].strip())
    else:
        if "_Misc" not in Meta:
            Meta["_Misc"] = []
        Meta["_Misc"].append(line.strip())

print("FEX Release {0}".format(sys.argv[1]))

Category = ""
Tag = ""
for item in sorted(Meta.items()):
    if item[0] == "_Misc":
        tag = "Misc"
    else:
        tag = item[0]

    category = tag.split("/")[0]
    if category != Category:
        Category = category
        Tag = ""
        print("")
        print("- " + category)
    if Tag != tag and tag != category:
        Tag = tag
        print("")
        print("  - " + tag.split("/")[1])

    for change in item[1]:
        if Tag == "":
            print("  - " + change)
        else:
            print("    - " + change)
