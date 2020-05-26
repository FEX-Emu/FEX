import json
import sys

# Print out enum values
def print_enums(ops, defines):
    output_file.write("#ifdef IROP_ENUM\n")
    output_file.write("enum IROps : uint8_t {\n")

    for op_key, op_vals in ops.items():
        output_file.write("\t\tOP_%s,\n" % op_key.upper())

    output_file.write("};\n")

    output_file.write("#undef IROP_ENUM\n")
    output_file.write("#endif\n\n")

# Print out struct definitions
def print_ir_structs(ops, defines):
    output_file.write("#ifdef IROP_STRUCTS\n")

    # Print out defines here
    for op_val in defines:
        output_file.write("\t%s;\n" % op_val)

    output_file.write("// Default structs\n")
    output_file.write("struct __attribute__((packed)) IROp_Header {\n")
    output_file.write("\tvoid* Data[0];\n")
    output_file.write("\tIROps Op;\n\n")
    output_file.write("\tuint8_t Size;\n")
    output_file.write("\tuint8_t NumArgs;\n")
    output_file.write("\tuint8_t ElementSize : 7;\n")
    output_file.write("\tbool HasDest : 1;\n")

    output_file.write("\ttemplate<typename T>\n")
    output_file.write("\tT const* C() const { return reinterpret_cast<T const*>(Data); }\n")
    output_file.write("\ttemplate<typename T>\n")
    output_file.write("\tT* CW() { return reinterpret_cast<T*>(Data); }\n")

    output_file.write("\tOrderedNodeWrapper Args[0];\n")

    output_file.write("};\n\n");

    output_file.write("struct __attribute__((packed)) IROp_Empty {\n")
    output_file.write("\tIROp_Header Header;\n")
    output_file.write("};\n\n")

    output_file.write("// User defined IR Op structs\n")
    for op_key, op_vals in ops.items():
        SSAArgs = 0
        HasArgs = False
        HasSSANames = False

        if ("SSAArgs" in op_vals):
            SSAArgs = int(op_vals["SSAArgs"])

        if ("Args" in op_vals and len(op_vals["Args"]) != 0):
            HasArgs = True

        if ("SSANames" in op_vals and len(op_vals["SSANames"]) != 0):
            HasSSANames = True

        if (HasArgs or SSAArgs != 0):
            output_file.write("struct __attribute__((packed)) IROp_%s {\n" % op_key)
            output_file.write("\tIROp_Header Header;\n\n")

            # SSA arguments have a hard requirement to appear after the header
            if (SSAArgs != 0):
                if (HasSSANames):
                    for i in range(0, SSAArgs):
                        output_file.write("\tOrderedNodeWrapper %s;\n" % (op_vals["SSANames"][i]));

                else:
                    output_file.write("private:\n")
                    for i in range(0, SSAArgs):
                        output_file.write("\tuint64_t : (sizeof(OrderedNodeWrapper) * 8);\n");
                    output_file.write("public:\n")

            if (HasArgs):
                output_file.write("\t// User defined data\n")

                # Print out arguments in IR Op
                for i in range(0, len(op_vals["Args"]), 2):
                    data_type = op_vals["Args"][i]
                    data_name = op_vals["Args"][i+1]
                    output_file.write("\t%s %s;\n" % (data_type, data_name))

            output_file.write("};\n")
        else:
            output_file.write("using IROp_%s = IROp_Empty;\n" % op_key)

        # Add a static assert that the IR ops must be pod
        output_file.write("static_assert(std::is_pod<IROp_%s>::value);\n\n" % op_key)

    output_file.write("#undef IROP_STRUCTS\n")
    output_file.write("#endif\n\n")

# Print out const expression to calculate IR Op sizes
def print_ir_sizes(ops, defines):
    output_file.write("#ifdef IROP_SIZES\n")

    output_file.write("constexpr std::array<size_t, IROps::OP_LAST + 1> IRSizes = {\n")
    for op_key, op_vals in ops.items():
        if ("Last" in op_vals):
            output_file.write("\t-1ULL,\n")
        else:
            output_file.write("\tsizeof(IROp_%s),\n" % op_key)

    output_file.write("};\n\n")

    output_file.write("// Make sure our array maps directly to the IROps enum\n")
    output_file.write("static_assert(IRSizes[IROps::OP_LAST] == -1ULL);\n\n")

    output_file.write("[[maybe_unused]] static size_t GetSize(IROps Op) { return IRSizes[Op]; }\n\n")

    output_file.write("std::string_view const& GetName(IROps Op);\n")
    output_file.write("uint8_t GetArgs(IROps Op);\n")
    output_file.write("FEXCore::IR::RegisterClassType GetRegClass(IROps Op);\n\n")
    output_file.write("bool HasSideEffects(IROps Op);\n")

    output_file.write("#undef IROP_SIZES\n")
    output_file.write("#endif\n\n")

def print_ir_reg_classes(ops, defines):
    output_file.write("#ifdef IROP_REG_CLASSES_IMPL\n")

    output_file.write("constexpr std::array<FEXCore::IR::RegisterClassType, IROps::OP_LAST + 1> IRRegClasses = {\n")
    for op_key, op_vals in ops.items():
        if ("Last" in op_vals):
            output_file.write("\tFEXCore::IR::InvalidClass,\n")
        else:
            Class = "Invalid"
            if ("HasDest" in op_vals and not ("DestClass" in op_vals)):
                sys.exit("IR op %s has destination with no destination class" % (op_key))

            if ("DestClass" in op_vals):
                Class = op_vals["DestClass"]
            output_file.write("\tFEXCore::IR::%sClass,\n" % Class)

    output_file.write("};\n\n")

    output_file.write("// Make sure our array maps directly to the IROps enum\n")
    output_file.write("static_assert(IRRegClasses[IROps::OP_LAST] == FEXCore::IR::InvalidClass);\n\n")

    output_file.write("FEXCore::IR::RegisterClassType GetRegClass(IROps Op) { return IRRegClasses[Op]; }\n\n")

    output_file.write("#undef IROP_REG_CLASSES_IMPL\n")
    output_file.write("#endif\n\n")

# Print out the name printer implementation
def print_ir_getname(ops, defines):
    output_file.write("#ifdef IROP_GETNAME_IMPL\n")
    output_file.write("constexpr std::array<std::string_view const, OP_LAST + 1> IRNames = {\n")
    for op_key, op_vals in ops.items():
        output_file.write("\t\"%s\",\n" % op_key)

    output_file.write("};\n\n")

    output_file.write("static_assert(IRNames[OP_LAST] == \"Last\");\n\n")

    output_file.write("std::string_view const& GetName(IROps Op) {\n")
    output_file.write("  return IRNames[Op];\n")
    output_file.write("}\n")

    output_file.write("#undef IROP_GETNAME_IMPL\n")
    output_file.write("#endif\n\n")

# Print out the number of SSA args that need to be RA'd
def print_ir_getraargs(ops, defines):
    output_file.write("#ifdef IROP_GETRAARGS_IMPL\n")

    output_file.write("constexpr std::array<uint8_t, OP_LAST + 1> IRArgs = {\n")
    for op_key, op_vals in ops.items():
        SSAArgs = 0
        if ("SSAArgs" in op_vals):
            SSAArgs = int(op_vals["SSAArgs"])

        if ("RAOverride" in op_vals):
            SSAArgs = int(op_vals["RAOverride"])

        output_file.write("\t%d,\n" % SSAArgs)

    output_file.write("};\n\n")

    output_file.write("uint8_t GetArgs(IROps Op) {\n")
    output_file.write("  return IRArgs[Op];\n")
    output_file.write("}\n")

    output_file.write("#undef IROP_GETRAARGS_IMPL\n")
    output_file.write("#endif\n\n")

def print_ir_hassideeffects(ops, defines):
    output_file.write("#ifdef IROP_HASSIDEEFFECTS_IMPL\n")

    output_file.write("constexpr std::array<uint8_t, OP_LAST + 1> SideEffects = {\n")
    for op_key, op_vals in ops.items():
        HasSideEffects = False
        if ("HasSideEffects" in op_vals):
            SSAArgs = op_vals["HasSideEffects"]

        output_file.write("\t%s,\n" % ("true" if SSAArgs else "false"))

    output_file.write("};\n\n")

    output_file.write("bool HasSideEffects(IROps Op) {\n")
    output_file.write("  return SideEffects[Op];\n")
    output_file.write("}\n")

    output_file.write("#undef IROP_HASSIDEEFFECTS_IMPL\n")
    output_file.write("#endif\n\n")

# Print out IR argument printing
def print_ir_arg_printer(ops, defines):
    output_file.write("#ifdef IROP_ARGPRINTER_HELPER\n")
    output_file.write("switch (IROp->Op) {\n")
    for op_key, op_vals in ops.items():
        if not ("Last" in op_vals):
            SSAArgs = 0
            HasArgs = False
            HasHelperArgs = False

            # Does this not want a printer?
            if ("ArgPrinter" in op_vals and op_vals["ArgPrinter"] == False):
                continue

            if ("SSAArgs" in op_vals):
                SSAArgs = int(op_vals["SSAArgs"])

            if ("Args" in op_vals and len(op_vals["Args"]) != 0):
                HasArgs = True

            if ("HelperArgs" in op_vals and len(op_vals["HelperArgs"]) != 0):
                HasHelperArgs = True

            output_file.write("case IROps::OP_%s: {\n" % op_key.upper())
            if (HasArgs or SSAArgs != 0):
                output_file.write("\tauto Op = IROp->C<IR::IROp_%s>();\n" % op_key)
                output_file.write("\t*out << \" \";\n")

                # Print SSA args first
                if (SSAArgs != 0):
                    for i in range(0, SSAArgs):
                        LastArg = (SSAArgs - i - 1) == 0 and not HasArgs
                        output_file.write("\tPrintArg(out, IR, Op->Header.Args[%d], RAPass);\n" % i)
                        if not (LastArg):
                            output_file.write("\t*out << \", \";\n")

                # Now print user defined arguments
                if (HasArgs):
                    ArgCount = len(op_vals["Args"])

                    for i in range(0, len(op_vals["Args"]), 2):
                        data_name = op_vals["Args"][i+1]
                        LastArg = (ArgCount - i - 2) == 0
                        CondArg2 = (", ", "")
                        output_file.write("\tPrintArg(out, IR, Op->%s);\n" % data_name)
                        if not (LastArg):
                            output_file.write("\t*out << \", \";\n")

            output_file.write("break;\n")
            output_file.write("}\n")


    output_file.write("#undef IROP_ARGPRINTER_HELPER\n")
    output_file.write("#endif\n")

# Print out IR allocator helpers
def print_ir_allocator_helpers(ops, defines):
    output_file.write("#ifdef IROP_ALLOCATE_HELPERS\n")

    output_file.write("\ttemplate <class T>\n")
    output_file.write("\tstruct Wrapper final {\n")
    output_file.write("\t\tT *first;\n")
    output_file.write("\t\tOrderedNode *Node; ///< Actual offset of this IR in ths list\n")
    output_file.write("\t\t\n")
    output_file.write("\t\toperator Wrapper<IROp_Header>() const { return Wrapper<IROp_Header> {reinterpret_cast<IROp_Header*>(first), Node}; }\n")
    output_file.write("\t\toperator OrderedNode *() { return Node; }\n")
    output_file.write("\t\toperator OpNodeWrapper () { return Node->Header.Value; }\n")
    output_file.write("\t};\n")

    output_file.write("\ttemplate <class T>\n")
    output_file.write("\tusing IRPair = Wrapper<T>;\n\n")

    output_file.write("\tIRPair<IROp_Header> AllocateRawOp(size_t HeaderSize) {\n")
    output_file.write("\t\tauto Op = reinterpret_cast<IROp_Header*>(Data.Allocate(HeaderSize));\n")
    output_file.write("\t\tmemset(Op, 0, HeaderSize);\n")
    output_file.write("\t\tOp->Op = IROps::OP_DUMMY;\n")
    output_file.write("\t\treturn IRPair<IROp_Header>{Op, CreateNode(Op)};\n")
    output_file.write("\t}\n\n")

    output_file.write("\ttemplate<class T, IROps T2>\n")
    output_file.write("\tT *AllocateOrphanOp() {\n")
    output_file.write("\t\tsize_t Size = FEXCore::IR::GetSize(T2);\n")
    output_file.write("\t\tauto Op = reinterpret_cast<T*>(Data.Allocate(Size));\n")
    output_file.write("\t\tmemset(Op, 0, Size);\n")
    output_file.write("\t\tOp->Header.Op = T2;\n")
    output_file.write("\t\treturn Op;\n")
    output_file.write("\t}\n\n")

    output_file.write("\ttemplate<class T, IROps T2>\n")
    output_file.write("\tIRPair<T> AllocateOp() {\n")
    output_file.write("\t\tsize_t Size = FEXCore::IR::GetSize(T2);\n")
    output_file.write("\t\tauto Op = reinterpret_cast<T*>(Data.Allocate(Size));\n")
    output_file.write("\t\tmemset(Op, 0, Size);\n")
    output_file.write("\t\tOp->Header.Op = T2;\n")
    output_file.write("\t\treturn IRPair<T>{Op, CreateNode(&Op->Header)};\n")
    output_file.write("\t}\n\n")

    output_file.write("\tuint8_t GetOpSize(OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(Data.Begin());\n")
    output_file.write("\t\tLogMan::Throw::A(HeaderOp->HasDest, \"Op %s has no dest\\n\", GetName(HeaderOp->Op));\n")
    output_file.write("\t\treturn HeaderOp->Size;\n")
    output_file.write("\t}\n\n")

    output_file.write("\tuint8_t GetOpElements(OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(Data.Begin());\n")
    output_file.write("\t\tLogMan::Throw::A(HeaderOp->HasDest, \"Op %s has no dest\\n\", GetName(HeaderOp->Op));\n")
    output_file.write("\t\treturn HeaderOp->Size / HeaderOp->ElementSize;\n")
    output_file.write("\t}\n\n")

    output_file.write("\tbool OpHasDest(OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(Data.Begin());\n")
    output_file.write("\t\treturn HeaderOp->HasDest;\n")
    output_file.write("\t}\n\n")

    # Generate helpers with operands
    for op_key, op_vals in ops.items():
        if not ("Last" in op_vals):
            SSAArgs = 0
            HasArgs = False
            HasHelperArgs = False
            HasDest = False
            HasFixedDestSize = False
            FixedDestSize = 0
            HasDestSize = False;
            NumElements = "1"
            DestSize = ""

            if ("SSAArgs" in op_vals):
                SSAArgs = int(op_vals["SSAArgs"])

            if ("Args" in op_vals and len(op_vals["Args"]) != 0):
                HasArgs = True

            if ("HelperArgs" in op_vals and len(op_vals["HelperArgs"]) != 0):
                HasHelperArgs = True

            if ("HelperGen" in op_vals and op_vals["HelperGen"] == False):
                continue;

            if ("HasDest" in op_vals and op_vals["HasDest"] == True):
                HasDest = True

            if ("FixedDestSize" in op_vals):
                HasFixedDestSize = True
                FixedDestSize = int(op_vals["FixedDestSize"])

            if ("DestSize" in op_vals):
                HasDestSize = True
                DestSize = op_vals["DestSize"]

            if ("NumElements" in op_vals):
                NumElements = op_vals["NumElements"]

            output_file.write("\tIRPair<IROp_%s> _%s(" % (op_key, op_key))

            # Output SSA args first
            if (SSAArgs != 0):
                for i in range(0, SSAArgs):
                    LastArg = (SSAArgs - i - 1) == 0 and not (HasArgs or HasHelperArgs)
                    CondArg2 = (", ", "")
                    output_file.write("OrderedNode *ssa%d%s" % (i, CondArg2[LastArg]))

            if (HasArgs or HasHelperArgs):
                ArgCount = 0
                Args = []
                if (HasArgs):
                    ArgCount += len(op_vals["Args"])
                    Args += op_vals["Args"]

                if (HasHelperArgs):
                    ArgCount += len(op_vals["HelperArgs"])
                    Args += op_vals["HelperArgs"]

                for i in range(0, ArgCount, 2):
                    data_type = Args[i]
                    data_name = Args[i+1]
                    LastArg = (ArgCount - i - 2) == 0
                    CondArg2 = (", ", "")

                    output_file.write("%s %s%s" % (data_type, data_name, CondArg2[LastArg]))

            output_file.write(") {\n")

            output_file.write("\t\tauto Op = AllocateOp<IROp_%s, IROps::OP_%s>();\n" % (op_key, op_key.upper()))
            output_file.write("\t\tOp.first->Header.NumArgs = %d;\n" % (SSAArgs))

            if (SSAArgs != 0):
                for i in range(0, SSAArgs):
                    output_file.write("\t\tOp.first->Header.Args[%d] = ssa%d->Wrapped(ListData.Begin());\n" % (i, i))
                    output_file.write("\t\tssa%d->AddUse();\n" % (i))

            if (HasArgs):
                for i in range(1, len(op_vals["Args"]), 2):
                    data_name = op_vals["Args"][i]
                    output_file.write("\t\tOp.first->%s = %s;\n" % (data_name, data_name))

            if (HasFixedDestSize):
                output_file.write("\t\tOp.first->Header.Size = %d;\n" % FixedDestSize)
            if (HasDestSize):
                output_file.write("\t\tOp.first->Header.Size = %s;\n" % DestSize)

            if (HasDest):
                # We can only infer a size if we have arguments
                if not (HasFixedDestSize or HasDestSize):
                    # We need to infer destination size
                    output_file.write("\t\tuint8_t InferSize = 0;\n")
                    if (SSAArgs != 0):
                        for i in range(0, SSAArgs):
                            output_file.write("\t\tuint8_t Size%d = GetOpSize(ssa%s);\n" % (i, i))
                            output_file.write("\t\tInferSize = std::max(InferSize, Size%d);\n" % (i))

                    output_file.write("\t\tOp.first->Header.Size = InferSize;\n")

            output_file.write("\t\tOp.first->Header.ElementSize = Op.first->Header.Size / (%s);\n" % NumElements)

            if (HasDest):
                output_file.write("\t\tOp.first->Header.HasDest = true;\n")

            output_file.write("\t\treturn Op;\n")
            output_file.write("\t}\n\n")

    output_file.write("#undef IROP_ALLOCATE_HELPERS\n")
    output_file.write("#endif\n")


# IR parser allocators
def print_ir_parser_allocator_helpers(ops, defines):
    output_file.write("#ifdef IROP_PARSER_ALLOCATE_HELPERS\n")

    # Generate helpers with operands
    for op_key, op_vals in ops.items():
        if not ("Last" in op_vals):
            SSAArgs = 0
            HasArgs = False
            HasDest = False
            HasFixedDestSize = False
            FixedDestSize = 0
            HasDestSize = False;
            NumElements = "1"
            DestSize = ""

            if ("SSAArgs" in op_vals):
                SSAArgs = int(op_vals["SSAArgs"])

            if ("Args" in op_vals and len(op_vals["Args"]) != 0):
                HasArgs = True

            if ("HelperGen" in op_vals and op_vals["HelperGen"] == False):
                continue;

            if ("HasDest" in op_vals and op_vals["HasDest"] == True):
                HasDest = True

            if ("FixedDestSize" in op_vals):
                HasFixedDestSize = True
                FixedDestSize = int(op_vals["FixedDestSize"])

            if ("DestSize" in op_vals):
                HasDestSize = True
                DestSize = op_vals["DestSize"]

            if ("NumElements" in op_vals):
                NumElements = op_vals["NumElements"]

            CondArg2 = (", ", "")

            output_file.write("\tIRPair<IROp_%s> _Parser_%s(FEXCore::IR::TypeDefinition _ParsedSize%s" %
                (op_key, op_key, CondArg2[(0 if (HasArgs or SSAArgs != 0) else 1)]))

            # Output SSA args first
            if (SSAArgs != 0):
                for i in range(0, SSAArgs):
                    LastArg = (SSAArgs - i - 1) == 0 and not (HasArgs)
                    output_file.write("OrderedNode *ssa%d%s" % (i, CondArg2[LastArg]))

            if (HasArgs):
                ArgCount = 0
                Args = []
                if (HasArgs):
                    ArgCount += len(op_vals["Args"])
                    Args += op_vals["Args"]

                for i in range(0, ArgCount, 2):
                    data_type = Args[i]
                    data_name = Args[i+1]
                    LastArg = (ArgCount - i - 2) == 0

                    output_file.write("%s %s%s" % (data_type, data_name, CondArg2[LastArg]))

            output_file.write(") {\n")

            output_file.write("\t\tauto Op = AllocateOp<IROp_%s, IROps::OP_%s>();\n" % (op_key, op_key.upper()))
            output_file.write("\t\tOp.first->Header.NumArgs = %d;\n" % (SSAArgs))

            if (SSAArgs != 0):
                for i in range(0, SSAArgs):
                    output_file.write("\t\tOp.first->Header.Args[%d] = ssa%d->Wrapped(ListData.Begin());\n" % (i, i))
                    output_file.write("\t\tssa%d->AddUse();\n" % (i))

            if (HasArgs):
                for i in range(1, len(op_vals["Args"]), 2):
                    data_name = op_vals["Args"][i]
                    output_file.write("\t\tOp.first->%s = %s;\n" % (data_name, data_name))

            output_file.write("\t\tOp.first->Header.Size = _ParsedSize.Bytes() * _ParsedSize.Elements();\n")
            output_file.write("\t\tOp.first->Header.ElementSize = _ParsedSize.Bytes();\n")

            if (HasDest):
                output_file.write("\t\tOp.first->Header.HasDest = true;\n")

            output_file.write("\t\treturn Op;\n")
            output_file.write("\t}\n\n")

    output_file.write("#undef IROP_PARSER_ALLOCATE_HELPERS\n")
    output_file.write("#endif\n")

# IR parser switch statement generation
def print_ir_parser_switch_helper(ops, defines):
    output_file.write("#ifdef IROP_PARSER_SWITCH_HELPERS\n")

    # Generate helpers with operands
    for op_key, op_vals in ops.items():
        if not ("Last" in op_vals):
            SSAArgs = 0
            HasArgs = False
            HasDest = False
            HasFixedDestSize = False
            FixedDestSize = 0
            HasDestSize = False;
            NumElements = "1"
            DestSize = ""

            if ("SSAArgs" in op_vals):
                SSAArgs = int(op_vals["SSAArgs"])

            if ("Args" in op_vals and len(op_vals["Args"]) != 0):
                HasArgs = True

            if ("SwitchGen" in op_vals and op_vals["SwitchGen"] == False):
                continue;

            if ("HasDest" in op_vals and op_vals["HasDest"] == True):
                HasDest = True

            if ("FixedDestSize" in op_vals):
                HasFixedDestSize = True
                FixedDestSize = int(op_vals["FixedDestSize"])

            if ("DestSize" in op_vals):
                HasDestSize = True
                DestSize = op_vals["DestSize"]

            if ("NumElements" in op_vals):
                NumElements = op_vals["NumElements"]

            CondArg2 = (", ", "")

            output_file.write("\tcase FEXCore::IR::IROps::OP_%s: {\n" % (op_key.upper()))

            # Output SSA args first
            if (SSAArgs != 0):
                for i in range(0, SSAArgs):
                    output_file.write("\t\tauto ssa_arg%d = DecodeValue<OrderedNode*>(Def.Args[%d]);\n" % (i, i))
                    output_file.write("\t\tif (!CheckPrintError(Def, ssa_arg%d.first)) return false;\n" % (i))

            if (HasArgs):
                ArgCount = 0
                Args = []
                if (HasArgs):
                    ArgCount += len(op_vals["Args"])
                    Args += op_vals["Args"]

                for i in range(0, ArgCount, 2):
                    data_type = Args[i]
                    data_name = Args[i+1]
                    LastArg = (ArgCount - i - 2) == 0

                    output_file.write("\t\tauto arg%d = DecodeValue<%s>(Def.Args[%d]);\n" % (SSAArgs + (i / 2), data_type, SSAArgs + (i / 2)))
                    output_file.write("\t\tif (!CheckPrintError(Def, arg%d.first)) return false;\n" % (SSAArgs + (i / 2)))

            output_file.write("\t\tDef.Node = _Parser_%s(Def.Size\n" % (op_key))

            if (SSAArgs != 0):
                for i in range(0, SSAArgs):
                    output_file.write("\t\t, ssa_arg%d.second\n" % (i))

            if (HasArgs):
                ArgCount = 0
                Args = []
                if (HasArgs):
                    ArgCount += len(op_vals["Args"])
                    Args += op_vals["Args"]

                for i in range(0, ArgCount, 2):
                    data_type = Args[i]
                    data_name = Args[i+1]
                    LastArg = (ArgCount - i - 2) == 0

                    output_file.write("\t\t, arg%d.second\n" % (SSAArgs + (i / 2)))

            output_file.write("\t\t);\n")

            output_file.write("\t\tSSANameMapper[Def.Definition] = Def.Node;\n")

            output_file.write("\t\tbreak;\n")
            output_file.write("\t}\n")

    output_file.write("#undef IROP_PARSER_SWITCH_HELPERS\n")
    output_file.write("#endif\n")
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

output_file = open(output_filename, "w")

print_enums(ops, defines)
print_ir_structs(ops, defines)
print_ir_sizes(ops, defines)
print_ir_reg_classes(ops, defines)
print_ir_getname(ops, defines)
print_ir_getraargs(ops, defines)
print_ir_hassideeffects(ops, defines)
print_ir_arg_printer(ops, defines)
print_ir_allocator_helpers(ops, defines)
print_ir_parser_allocator_helpers(ops, defines)
print_ir_parser_switch_helper(ops, defines)

output_file.close()
