import collections
import json
import sys

OpClasses = collections.OrderedDict()


def get_ir_classes(ops, defines):
    global OpClasses

    for op_class, opslist in ops.items():
        if not (op_class in OpClasses):
            OpClasses[op_class] = []

        for op, op_val in opslist.items():
            OpClasses[op_class].append([op, op_val])

    # Sort the dictionary after we are done parsing it
    OpClasses = collections.OrderedDict(sorted(OpClasses.items()))


def print_ir_op_index():
    output_file.write("# Index\n")
    output_file.write("## Op Classes\n")
    for class_key, class_value in OpClasses.items():
        output_file.write("- [%s](#%s)\n\n" % (class_key, class_key))

    output_file.write("## Definitions\n")
    output_file.write("- [Defines](#Defines)\n\n")


def print_ir_ops():
    for class_key, class_value in OpClasses.items():
        output_file.write("# %s\n\n" % (class_key))
        for op in class_value:
            op_key = op[0]
            op_vals = op[1]
            output_file.write("## %s\n" % (op_key))

            output_file.write(">")
            output_file.write(op_key)

            output_file.write("\n\n")

            if "Desc" in op_vals:
                desc = op_vals["Desc"]
                if isinstance(desc, list):
                    for line in desc:
                        output_file.write("%s\n\n" % line)
                else:
                    output_file.write("%s\n" % op_vals["Desc"])
            else:
                output_file.write("XXX: Missing op desc!\n")


def print_ir_defines(defines):
    output_file.write("## Defines\n")
    output_file.write("```cpp\n")
    for define in defines:
        output_file.write("%s\n" % (define))
    output_file.write("```\n")


if len(sys.argv) < 3:
    sys.exit()

output_filename = sys.argv[2]
json_file = open(sys.argv[1], "r")
json_text = json_file.read()
json_file.close()

json_object = json.loads(json_text)
json_object = {k.upper(): v for k, v in json_object.items()}

ops = json_object["OPS"]
defines = json_object["DEFINES"]

get_ir_classes(ops, defines)

output_file = open(output_filename, "w")

print_ir_op_index()

output_file.write("# IR documentation\n\n")

print_ir_ops()

print_ir_defines(defines)

output_file.close()
