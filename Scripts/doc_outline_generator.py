#!/usr/bin/python3

import re
from pathlib import Path
import sys

if (len(sys.argv) != 4):
    print("doc_outline_generator GIT_DIR SRC_DIR LINK_PREFIX")
    sys.exit(-2)

Base = Path(sys.argv[1])
Root = Path(sys.argv[2])
Prefix = sys.argv[3]

Paths = []

for path in Root.rglob('*.c'):
    Paths.append(path)

for path in Root.rglob('*.cpp'):
    Paths.append(path)

for path in Root.rglob('*.cc'):
    Paths.append(path)

for path in Root.rglob('*.h'):
    Paths.append(path)

for path in Root.rglob('*.hpp'):
    Paths.append(path)

CategoryLabels = { }
MetaLabels = { }
GlossaryLabels = { }

Meta = { }
Desc = { }

for path in Paths:
    with path.open() as file:
        txt = file.read()
        x = re.findall("\$info\$([^\$]*)\$end_info\$", txt)
        if x:
            for entry in x[0].strip().split("\n"):
                name = entry.split(":")[0].strip();
                val = entry.split(":")[1].strip();
                if name == "category":
                    cat_name = val.split("~")[0].strip();
                    cat_val = val.split("~")[1].strip();
                    CategoryLabels[cat_name] = cat_val
                elif name == "meta":
                    meta_name = val.split("~")[0].strip();
                    meta_val = val.split("~")[1].strip();
                    MetaLabels[meta_name] = meta_val
                elif name == "glossary":
                    glossary_name = val.split("~")[0].strip();
                    glossary_val = val.split("~")[1].strip();
                    GlossaryLabels[glossary_name] = glossary_val
                elif name == "tags":
                    for meta_name in val.split(","):
                        if meta_name.strip() not in Meta:
                            Meta[meta_name.strip()] = []
                        Meta[meta_name.strip()].append(path)
                elif name == "desc":
                    Desc[path] = val;
                else:
                    print("Error")
                    sys.exit(-1)

print("## " + Root.name)
print("")

if (GlossaryLabels):
    print ("### Glossary")
    print("")
    for item in GlossaryLabels.items():
        print("- " + item[0] + ": " + item[1])
    print("")
    print("")

Category = ""
for item in sorted(Meta.items()):
    meta = item[0]
    category = meta.split("|")[0]
    topic = meta.split("|")[1]
    if Category != category:
        if Category != "":
            print("")
            print("")
        Category = category
        print("### " + Category)
        if Category in CategoryLabels:
            print(CategoryLabels[Category])
        print("")

    print("#### " + topic)
    if meta in MetaLabels:
            print(MetaLabels[meta])

    for path in sorted(item[1]):
        if path in Desc:
            print("- [" + path.name + "](" + Prefix + path.relative_to(Base).as_posix() + ")" + ": " + Desc[path])
        else:
            print("- [" + path.name + "](" + Prefix + path.relative_to(Base).as_posix() + ")")
    print("")
