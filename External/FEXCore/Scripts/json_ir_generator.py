import json
import sys
import textwrap

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


def constraint_find_base_reg(json_arch_constraints_object, class_name, reg):
    if (class_name == "GPR" or class_name == "FPR"):
        return reg

    constraints_register_classes = json_arch_constraints_object["REGCLASSES"]
    constraints_reg = constraints_register_classes["GPRPair"]
    # If both income pairs match the definition then that is the match
    # Pair registers have a handicap that they don't interfere with each other, only GPRs
    # This means that the base register will always be unique
    # Just check both to ensure correct IR generation
    for op_val in constraints_reg:
        if (op_val[0] == reg[0] and op_val[1] == reg[1]):
            return reg[0]

    # Someone hecked up
    sys.exit("Constraint pair class '[%s, %s]' doesn't exist" % (reg[0], reg[1]))

# Print out architecture GPRs
def constraint_print_ir_defines(json_object, json_arch_constraints_object, ops, defines):
    MAX_PHYSICAL = 4
    if (True):
        output_constraints_file.write("#ifdef IR_CONSTRAINT_REGS\n")
        output_constraints_file.write("namespace FEXCore::IR::Arch {\n")

        # Print out defines here
        for op_val in defines:
            output_constraints_file.write("\t%s;\n" % op_val)

        output_constraints_file.write("\n\n")

        # Print out the register classes
        constraints_register_classes = json_arch_constraints_object["REGCLASSES"]

        if (True):
            # GPRs first
            constraints_reg = constraints_register_classes["GPR"]
            output_constraints_file.write("const std::array<uint8_t, %d> *GetGPRClass();\n" % len(constraints_reg))

            # FPRs
            constraints_reg = constraints_register_classes["FPR"]
            output_constraints_file.write("const std::array<uint8_t, %d> *GetFPRClass();\n" % len(constraints_reg))

            # GPR Pair
            constraints_reg = constraints_register_classes["GPRPair"]
            output_constraints_file.write("const std::array<std::pair<uint8_t, uint8_t>, %d> *GetGPRPairClass();\n" % len(constraints_reg))

        if (True):
            output_constraints_file.write(textwrap.dedent("""\
                // This is a 96bit struct atm
                // This fits in two GPRs on return
                struct OpConstraints {
                    // Flags from the constraints file
                    uint16_t Flags;
                    // Number of SSA temps that an op needs
                    uint8_t NumTempsGPR;
                    uint8_t NumTempsFPR;
                    uint8_t NumTempsGPRPair;
                    // Physical register source assignment that an op needs
                    // 0: Dst, 1-4: Srcs
                    // Top 2bits: Reg Class {GPR, FPR, GPRPair}
                    // Lower 6bits: Physical Register base
                    // RA interference handling figures out the GPR<->GPRPair interaction
                    uint8_t SrcPhysicalAssignment[5];
                    // Physical registers that are needed for the op
                    // Undefined what those registers are for, just that they are necessary
                    // Same 2+6bit packing as above
                    uint8_t NeedsPhysicalRegisters[%d];
                };

                OpConstraints GetOpConstraints(IROps Op);
                """ % (MAX_PHYSICAL)))

        output_constraints_file.write("}\n")
        output_constraints_file.write("#undef IR_CONSTRAINT_REGS\n")
        output_constraints_file.write("#endif\n")

    if (True):
        output_constraints_file.write("#ifdef IR_CONSTRAINT_REGS_IMPL\n")
        output_constraints_file.write("namespace FEXCore::IR::Arch {\n")

        if (True):
            # GPRs first
            constraints_reg = constraints_register_classes["GPR"]
            output_constraints_file.write("static const std::array<uint8_t, %d> Regs_GPR = {\n" % len(constraints_reg))
            for op_val in constraints_reg:
                output_constraints_file.write("\tFEXCore::IR::Arch::%s,\n" % op_val)

            output_constraints_file.write("};\n\n")
            output_constraints_file.write("const std::array<uint8_t, %d> *GetGPRClass() { return &FEXCore::IR::Arch::Regs_GPR; }\n" % len(constraints_reg))

            # FPRs
            constraints_reg = constraints_register_classes["FPR"]
            output_constraints_file.write("static const std::array<uint8_t, %d> Regs_FPR = {\n" % len(constraints_reg))
            for op_val in constraints_reg:
                output_constraints_file.write("\tFEXCore::IR::Arch::%s,\n" % op_val)

            output_constraints_file.write("};\n\n")
            output_constraints_file.write("const std::array<uint8_t, %d> *GetFPRClass() { return &FEXCore::IR::Arch::Regs_FPR; }\n" % len(constraints_reg))

            # GPR Pair
            constraints_reg = constraints_register_classes["GPRPair"]
            output_constraints_file.write("static const std::array<std::pair<uint8_t, uint8_t>, %d> Regs_GPRPair = {{\n" % len(constraints_reg))
            for op_val in constraints_reg:
                output_constraints_file.write("\t{ FEXCore::IR::Arch::%s, FEXCore::IR::Arch::%s },\n" % (op_val[0], op_val[1]))

            output_constraints_file.write("}};\n\n")
            output_constraints_file.write("const std::array<std::pair<uint8_t, uint8_t>, %d> *GetGPRPairClass() { return &FEXCore::IR::Arch::Regs_GPRPair; }\n" % len(constraints_reg))

        if (True):
            output_constraints_file.write(textwrap.dedent("""\
                const OpConstraints DefaultConstraint{};
                static const std::array<OpConstraints, IROps::OP_LAST + 1> OpConstraintsArray = {
                """))
            ir_ops = json_object["OPS"]
            for op_key, op_vals in ops.items():
                if not (op_key in ir_ops):
                    sys.exit("Constraint IR op %s doesn't exist in default table" % (op_key))

            for op_key, op_vals in ir_ops.items():
                if (op_key in ops):
                    op_constraints = ops[op_key]

                    num_temps_gpr = 0
                    num_temps_fpr = 0
                    num_temps_gprpair = 0

                    needs_specific_physicals = False
                    if ("TempCountGPR" in op_constraints):
                        num_temps_gpr = int(op_constraints["TempCountGPR"])
                    if ("TempCountFPR" in op_constraints):
                        num_temps_fpr = int(op_constraints["TempCountFPR"])
                    if ("TempCountGPRPair" in op_constraints):
                        num_temps_gprpair = int(op_constraints["TempCountGPRPair"])

                    if ("NeedsPhysical" in op_constraints):
                        needs_specific_physicals = len(op_constraints["NeedsPhysical"]) > 0

                    # OpConstraints layout
                    output_constraints_file.write("\t{ // IROps::OP_%s\n" % (op_key.upper()))
                    # Flags
                    output_constraints_file.write("\t\t.Flags = 0\n")
                    if ("Dest_Is_SSA0" in op_constraints and op_constraints["Dest_Is_SSA0"]):
                        output_constraints_file.write("\t\t\t| Constraint_Dest_Is_Src0\n")
                    if ("Physical_Dest" in op_constraints):
                        output_constraints_file.write("\t\t\t| Constraint_Dest_Is_Physical\n")
                    if ("Physical_ssa0" in op_constraints):
                        output_constraints_file.write("\t\t\t| Constraint_Src0_Is_Physical\n")
                    if ("Physical_ssa1" in op_constraints):
                        output_constraints_file.write("\t\t\t| Constraint_Src1_Is_Physical\n")
                    if ("Physical_ssa2" in op_constraints):
                        output_constraints_file.write("\t\t\t| Constraint_Src2_Is_Physical\n")
                    if ("Physical_ssa3" in op_constraints):
                        output_constraints_file.write("\t\t\t| Constraint_Src3_Is_Physical\n")
                    if (num_temps_gpr or num_temps_fpr or num_temps_gprpair):
                        output_constraints_file.write("\t\t\t| Constraint_Needs_Temps\n")
                    if (needs_specific_physicals):
                        output_constraints_file.write("\t\t\t| Constraint_Needs_Physicals\n")

                    if ("LateKill" in op_constraints):
                        for kill in op_constraints["LateKill"]:
                            ssa_num = kill.replace("ssa", "")
                            output_constraints_file.write("\t\t\t| Constraint_Src%d_Is_LateKill\n" % int(ssa_num))

                    output_constraints_file.write("\t\t,\n")
                    # NumTemps
                    output_constraints_file.write("\t\t.NumTempsGPR     = %d,\n" % num_temps_gpr)
                    output_constraints_file.write("\t\t.NumTempsFPR     = %d,\n" % num_temps_fpr)
                    output_constraints_file.write("\t\t.NumTempsGPRPair = %d,\n" % num_temps_gprpair)

                    # SrcPhysicalAssignment
                    output_constraints_file.write("\t\t.SrcPhysicalAssignment = {\n")
                    if ("Physical_Dest" in op_constraints):
                        physical_dest = op_constraints["Physical_Dest"]
                        output_constraints_file.write("\t\t\t(FEXCore::IR::%sClass.Val << 6) | %s, // dest\n" % (physical_dest[0],
                            constraint_find_base_reg(json_arch_constraints_object, physical_dest[0], physical_dest[1])))
                    else:
                        output_constraints_file.write("\t\t\t0, // dest\n")

                    for i in range(0, 4):
                        name = "Physical_ssa" + str(i)
                        if (name in op_constraints):
                            physical_src = op_constraints[name]
                            output_constraints_file.write("\t\t\t(FEXCore::IR::%sClass.Val << 6) | %s, // ssa%d\n" % (physical_src[0],
                                constraint_find_base_reg(json_arch_constraints_object, physical_src[0], physical_src[1]), i))
                        else:
                            output_constraints_file.write("\t\t\t0, // ssa%d\n" % i)
                    output_constraints_file.write("\t\t},\n")

                    # NeedsPhysicalRegisters
                    output_constraints_file.write("\t\t.NeedsPhysicalRegisters = {\n")
                    num_physical_needs = 0
                    if ("NeedsPhysical" in op_constraints):
                        physical_needs = op_constraints["NeedsPhysical"]
                        for physical_need in physical_needs:
                            output_constraints_file.write("\t\t\t(FEXCore::IR::%sClass.Val << 6) | %s, // Need %d\n" % (physical_need[0],
                                constraint_find_base_reg(json_arch_constraints_object, physical_need[0], physical_need[1]), num_physical_needs))
                            num_physical_needs += 1

                    for i in range(num_physical_needs, MAX_PHYSICAL):
                        output_constraints_file.write("\t\t\t255, // Need %d\n" % i)

                    output_constraints_file.write("\t\t},\n")

                    output_constraints_file.write("\t},\n")

                else:
                    output_constraints_file.write("\tDefaultConstraint,\n")

            output_constraints_file.write("};\n")

            output_constraints_file.write(textwrap.dedent("""\
                OpConstraints GetOpConstraints(IROps Op) {
                    return OpConstraintsArray[Op];
                }
                """))

        output_constraints_file.write("}\n")
        output_constraints_file.write("#undef IR_CONSTRAINT_REGS_IMPL\n")
        output_constraints_file.write("#endif\n")

if (len(sys.argv) < 4):
    sys.exit()

output_filename = sys.argv[2]
output_constraints_filename = sys.argv[4]

# Load the base JSON file
json_file = open(sys.argv[1], "r")
json_text = json_file.read()
json_file.close()

json_object = json.loads(json_text)
json_object = {k.upper(): v for k, v in json_object.items()}

# Load the provided JSON arch constraints

json_arch_constraints_file = open(sys.argv[3], "r")
json_arch_constraints_text = json_arch_constraints_file.read()
json_arch_constraints_file.close()

json_arch_constraints_object = json.loads(json_arch_constraints_text)
json_arch_constraints_object = {k.upper(): v for k, v in json_arch_constraints_object.items()}

ops = json_object["OPS"]
defines = json_object["DEFINES"]

constraints_ops = json_arch_constraints_object["OPS"]
constraints_defines = json_arch_constraints_object["DEFINES"]

output_file = open(output_filename, "w")
output_constraints_file = open(output_constraints_filename, "w")

print_enums(ops, defines)
print_ir_structs(ops, defines)
print_ir_sizes(ops, defines)
print_ir_reg_classes(ops, defines)
print_ir_getname(ops, defines)
print_ir_getraargs(ops, defines)
print_ir_arg_printer(ops, defines)
print_ir_allocator_helpers(ops, defines)

# Constraints
constraint_print_ir_defines(json_object, json_arch_constraints_object, constraints_ops, constraints_defines)

output_file.close()
output_constraints_file.close()
