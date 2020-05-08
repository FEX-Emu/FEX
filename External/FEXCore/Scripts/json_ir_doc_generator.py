import collections
import json
import sys

OpClasses = collections.OrderedDict()

def get_ir_classes(ops, defines):
    global OpClasses

    for op_key, op_vals in ops.items():
        if not ("Last" in op_vals):
            OpClass = "#Unknown"

            if ("OpClass" in op_vals):
                OpClass = op_vals["OpClass"]

            if not (OpClass in OpClasses):
                OpClasses[OpClass] = []

            OpClasses[OpClass].append([op_key, op_vals])

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
            HasDest = ("HasDest" in op_vals and op_vals["HasDest"] == True)
            HasSSAArgs = ("SSAArgs" in op_vals and len(op_vals["SSAArgs"]) > 0)
            HasSSAArgNames = "SSANames" in op_vals
            HasArgs = "Args" in op_vals
            SSAArgsCount = 0
            ArgCount = 0
            if (HasSSAArgs):
                SSAArgsCount = int(op_vals["SSAArgs"])
            if (HasArgs):
                ArgCount = len(op_vals["Args"])

            TotalArgsCount = SSAArgsCount + (ArgCount / 2)

            output_file.write(">")
            if (HasDest):
                output_file.write("%dest = ")

            output_file.write("%s " % op_key)

            ArgComma = (", ", "")
            if (HasSSAArgs):
                for i in range(0, SSAArgsCount):
                    FinalArg = (i + 1) == TotalArgsCount
                    if (HasSSAArgNames):
                        output_file.write("%%%s%s" % (op_vals["SSANames"][i], ArgComma[FinalArg]))
                    else:
                        output_file.write("%%ssa%d%s" % (i, ArgComma[FinalArg]))

            if (HasArgs):
                Args = op_vals["Args"]
                for i in range(0, ArgCount, 2):
                    FinalArg = ((i / 2) + SSAArgsCount + 1) == TotalArgsCount
                    data_type = Args[i]
                    data_name = Args[i + 1]
                    output_file.write("\<%s %s\>%s" % (data_type, data_name, ArgComma[FinalArg]))

            output_file.write("\n\n")

            if ("Desc" in op_vals):
                desc = op_vals["Desc"]
                if (isinstance(desc, list)):
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

if (len(sys.argv) < 3):
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
