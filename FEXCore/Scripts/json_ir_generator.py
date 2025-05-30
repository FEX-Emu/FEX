#!/bin/python3
import json
import sys
from dataclasses import dataclass, field
import textwrap

def ExitError(msg):
    print(msg)
    sys.exit(-1)

@dataclass
class IRType:
    IRName: str
    CXXName: str
    def __init__(self, IRName, CXXName):
        self.IRName = IRName
        self.CXXName = CXXName

@dataclass
class OpArgument:
    Type: str
    IsSSA: bool
    Temporary: bool
    Name: str
    NameWithPrefix: str
    DefaultInitializer: str

    def __init__(self):
        self.Type = None
        self.IsSSA = False
        self.Temporary = False
        self.Name = None
        self.NameWithPrefix = None
        self.DefaultInitializer = None
        return

    def print(self):
        attrs = vars(self)
        print(", ".join("%s: %s" % item for item in attrs.items()))

@dataclass
class OpDefinition:
    Name: str
    HasDest: bool
    DestType: str
    DestSize: str
    ElementSize: str
    OpClass: str
    HasSideEffects: bool
    ImplicitFlagClobber: bool
    RAOverride: int
    SwitchGen: bool
    ArgPrinter: bool
    SSAArgNum: int
    NonSSAArgNum: int
    DynamicDispatch: bool
    LoweredX87: bool
    JITDispatch: bool
    JITDispatchOverride: str
    TiedSource: int
    Arguments: list
    EmitValidation: list
    Desc: list

    def __init__(self):
        self.Name = None
        self.HasDest = False
        self.DestType = None
        self.DestSize = None
        self.ElementSize = None
        self.OpClass = None
        self.OpSize = 0
        self.HasSideEffects = False
        self.ImplicitFlagClobber = False
        self.RAOverride = -1
        self.SwitchGen = True
        self.ArgPrinter = True
        self.SSAArgNum = 0
        self.NonSSAArgNum = 0
        self.DynamicDispatch = False
        self.LoweredX87 = False
        self.JITDispatch = True
        self.JITDispatchOverride = None
        self.TiedSource = -1
        self.Arguments = []
        self.EmitValidation = []
        self.Desc = []
        return

    def print(self):
        attrs = vars(self)
        print(", ".join("%s: %s" % item for item in attrs.items()))

IRTypesToCXX = {}
CXXTypeToIR = {}
IROps = []

IROpNameMap = {}

def is_ssa_type(type):
    if (type == "SSA" or
       type == "GPR" or
       type == "GPRPair" or
       type == "FPR"):
       return True
    return False

def parse_irtypes(irtypes):
    for op_key, op_val in irtypes.items():
        IRTypesToCXX[op_key] = IRType(op_key, op_val)
        CXXTypeToIR[op_val] = IRType(op_key, op_val)

def parse_ops(ops):
    for op_class, opslist in ops.items():
        for op, op_val in opslist.items():
            if "Ignore" in op_val:
                # Skip these
                continue

            OpDef = OpDefinition()

            # Check if we have a destination
            # Only happens if the IR name contains `=`
            EqualSplit = op.split("=", 1)

            RHS = EqualSplit[0].strip()
            if len(EqualSplit) > 1:
                LHS = EqualSplit[0].strip()
                RHS = EqualSplit[1].strip()

                if ":" in LHS:
                    # Named destinations. This is a hack, but so is the entire
                    # multi-destination support bolten onto the old IR...
                    #
                    # Named destinations require side effects because they break
                    # SSA hard. Validate that.
                    assert("HasSideEffects" in op_val and op_val["HasSideEffects"])

                    for Dest in LHS.split(","):
                        Dest = Dest.strip()
                        DType, Name = Dest.split(":$")

                        # If the destination appears also as a source, it is
                        # read-modify-write.
                        if Dest in RHS:
                            # Turn RMW into an in/out source
                            RHS = RHS.replace(Dest.strip(), f"{DType}:$Inout{Name}")
                        else:
                            # Turn named destinations into an out source.
                            RHS += f", {DType}:$Out{Name}"
                else:
                    # Single anonymous destination
                    if LHS not in ["SSA", "GPR", "GPRPair", "FPR"]:
                        ExitError(f"Unknown destination class type {LHS}. Needs to be one of SSA, GPR, GPRPair, FPR")

                    OpDef.HasDest = True
                    OpDef.DestType = LHS

            # IR Op needs to start with a name
            RHS = RHS.split(" ", 1)

            if len(RHS) < 1:
                ExitError("Missing IR op name. Needs to be a string")

            # Set the op name
            OpDef.Name = RHS[0]

            # Parse the arguments
            if len(RHS) > 1:
                Arguments = RHS[1].strip().split(",")
                for Argument in Arguments:
                    Argument = Argument.strip()
                    OpArg = OpArgument()

                    Split = Argument.split(":", 1)
                    if len(Split) != 2:
                        ExitError("Error parsing argument. Missing Type and name colon split")

                    # Type is the first argument
                    OpArg.Type = Split[0]

                    # Validate typing is in our type map
                    if not OpArg.Type in IRTypesToCXX:
                        ExitError("IR type {} isn't in IR type map. From IR op {}, argument {}".format(OpArg.Type, OpDef.Name, Argument))

                    # Style is the first byte of the name
                    if Split[1][0] == "#":
                        OpArg.Temporary = True
                        OpArg.IsSSA = False
                    elif Split[1][0] == "$":
                        OpArg.Temporary = False
                        OpArg.IsSSA = is_ssa_type(OpArg.Type)
                        if OpArg.IsSSA:
                            OpDef.SSAArgNum = OpDef.SSAArgNum + 1
                        else:
                            OpDef.NonSSAArgNum = OpDef.NonSSAArgNum + 1
                    else:
                        ExitError("IR Op {} missing value argument style specifier. Needs to be one of {{#, $}}".format(OpDef.Name))

                    Prefix = Split[1][0]
                    ArgName = Split[1][1:]
                    NameWithPrefix = Prefix + ArgName

                    if len(ArgName) == 0:
                        ExitError("Argument is missing variable name")

                    DefaultInit = ArgName.split("{", 1)
                    if len(DefaultInit) > 1:
                        # We have a default initializer, need to do some more work
                        # First argument will still be the argument name
                        ArgName = DefaultInit[0].strip()
                        NameWithPrefix = Prefix + ArgName
                        # Second argument will be the default initializer
                        # Since we stripped the opening curly brace then it'll end with a closing brace
                        if DefaultInit[1][-1] != "}":
                            ExitError("IR op {} Argument {} is missing closing curly brace in default initializer?".format(OpDef.Name, ArgName))

                        OpArg.DefaultInitializer = DefaultInit[1][:-1]

                    # If SSA type then we can generate validation for this op
                    if (OpArg.IsSSA and
                        (OpArg.Type == "GPR" or
                        OpArg.Type == "GPRPair" or
                        OpArg.Type == "FPR")):
                        OpDef.EmitValidation.append(f"GetOpRegClass({ArgName}) == InvalidClass || WalkFindRegClass({ArgName}) == {OpArg.Type}Class")

                    OpArg.Name = ArgName
                    OpArg.NameWithPrefix = NameWithPrefix
                    OpDef.Arguments.append(OpArg)

            # Additional metadata
            if "DestSize" in op_val:
                OpDef.DestSize = op_val["DestSize"]

            if "ElementSize" in op_val:
                OpDef.ElementSize = op_val["ElementSize"]

            if len(op_class):
                OpDef.OpClass = op_class

            if "HasSideEffects" in op_val:
                OpDef.HasSideEffects = bool(op_val["HasSideEffects"])

            if "ImplicitFlagClobber" in op_val:
                OpDef.ImplicitFlagClobber = bool(op_val["ImplicitFlagClobber"])

            if "ArgPrinter" in op_val:
                OpDef.ArgPrinter = bool(op_val["ArgPrinter"])

            if "RAOverride" in op_val:
                OpDef.RAOverride = int(op_val["RAOverride"])

            if "SwitchGen" in op_val:
                OpDef.SwitchGen = op_val["SwitchGen"]

            if "EmitValidation" in op_val:
                OpDef.EmitValidation.extend(op_val["EmitValidation"])

            if "Desc" in op_val:
                OpDef.Desc = op_val["Desc"]

            if "DynamicDispatch" in op_val:
                OpDef.DynamicDispatch = bool(op_val["DynamicDispatch"])

            if "JITDispatch" in op_val:
                OpDef.JITDispatch = bool(op_val["JITDispatch"])

            if "JITDispatchOverride" in op_val:
                OpDef.JITDispatchOverride = op_val["JITDispatchOverride"]

            if "X87" in op_val:
                OpDef.LoweredX87 = op_val["X87"]

                # X87 implies !JITDispatch
                assert("JITDispatch" not in op_val)
                OpDef.JITDispatch = False

            if "TiedSource" in op_val:
                OpDef.TiedSource = op_val["TiedSource"]

            # Do some fixups of the data here
            if len(OpDef.EmitValidation) != 0:
                for i in range(len(OpDef.EmitValidation)):
                    # Patch up all the argument names
                    for Arg in OpDef.Arguments:
                        # Temporary ops just replace all instances no prefix variant
                        OpDef.EmitValidation[i] = OpDef.EmitValidation[i].replace(Arg.NameWithPrefix, Arg.Name)

            #OpDef.print()

            # Error on duplicate op
            if OpDef.Name in IROpNameMap:
                ExitError("Duplicate Op defined! {}".format(OpDef.Name))

            IROps.append(OpDef)
            IROpNameMap[OpDef.Name] = 1

# Print out enum values
def print_enums():
    output_file.write("#ifdef IROP_ENUM\n")
    output_file.write("enum IROps : uint16_t {\n")

    for op in IROps:
        output_file.write("\tOP_{},\n" .format(op.Name.upper()))

    output_file.write("};\n")

    output_file.write("#undef IROP_ENUM\n")
    output_file.write("#endif\n\n")

def print_ir_structs(defines):
    output_file.write("#ifdef IROP_STRUCTS\n")

    # Print out defines here
    for op_val in defines:
        if op_val:
            output_file.write("\t%s;\n" % op_val)
        else:
            output_file.write("\n")

    # Emit the default struct first
    output_file.write("// Default structs\n")
    output_file.write("struct __attribute__((packed)) IROp_Header {\n")
    output_file.write("\tvoid* Data[0];\n")
    output_file.write("\tIROps Op;\n\n")
    output_file.write("\tIR::OpSize Size;\n")
    output_file.write("\tIR::OpSize ElementSize;\n")

    output_file.write("\ttemplate<typename T>\n")
    output_file.write("\tT const* C() const { return reinterpret_cast<T const*>(Data); }\n")
    output_file.write("\ttemplate<typename T>\n")
    output_file.write("\tT* CW() { return reinterpret_cast<T*>(Data); }\n")

    output_file.write("\tOrderedNodeWrapper Args[0];\n")

    output_file.write("};\n\n");
    output_file.write("static_assert(sizeof(IROp_Header) == sizeof(uint32_t), \"IROp_Header should be 32-bits in size\");\n\n");

    # Now the user defined types
    output_file.write("// User defined IR Op structs\n")
    for op in IROps:
        output_file.write("struct __attribute__((packed)) IROp_{} {{\n".format(op.Name))
        output_file.write("\tIROp_Header Header;\n")

        # SSA arguments have a hard requirement to appear after the header
        if op.SSAArgNum > 0:
            output_file.write("\t// SSA arguments\n")

            # Walk the SSA arguments and place them in order of declaration
            for arg in op.Arguments:
                if arg.IsSSA:
                    output_file.write("\tOrderedNodeWrapper {};\n".format(arg.Name));

        # Non-SSA arguments are also placed in order of declaration, after SSA though
        if op.NonSSAArgNum > 0:
            output_file.write("\t// Non-SSA arguments\n")
            for arg in op.Arguments:
                if not arg.Temporary and not arg.IsSSA:
                    CType = IRTypesToCXX[arg.Type].CXXName
                    output_file.write("\t{} {};\n".format(CType, arg.Name));

        output_file.write("\tstatic constexpr IROps OPCODE = OP_{};\n".format(op.Name.upper()))


        if op.SSAArgNum > 0:
            output_file.write("\t// Get index of argument by name\n")
            SSAArg = 0
            for arg in op.Arguments:
                if arg.IsSSA:
                    output_file.write("\tstatic constexpr size_t {}_Index = {};\n".format(arg.Name, SSAArg))
                    SSAArg = SSAArg + 1


        output_file.write("};\n")

        # Add a static assert that the IR ops must be pod
        output_file.write("static_assert(std::is_trivially_copyable_v<IROp_{}>);\n".format(op.Name))
        output_file.write("static_assert(std::is_standard_layout_v<IROp_{}>);\n\n".format(op.Name))

    output_file.write("#undef IROP_STRUCTS\n")
    output_file.write("#endif\n\n")

# Print out const expression to calculate IR Op sizes
def print_ir_sizes():
    output_file.write("#ifdef IROP_SIZES\n")

    output_file.write("constexpr std::array<size_t, IROps::OP_LAST + 1> IRSizes = {\n")
    for op in IROps:
        if op.Name == "Last":
            output_file.write("\t-1ULL,\n")
        else:
            output_file.write(f"\tsizeof(IROp_{op.Name}),\n")

    output_file.write(textwrap.dedent("""
    };

    // Make sure our array maps directly to the IROps enum
    static_assert(IRSizes[IROps::OP_LAST] == -1ULL);

    [[maybe_unused, nodiscard]] static size_t GetSize(IROps Op) { return IRSizes[Op]; }
    [[nodiscard, gnu::const, gnu::visibility("default")]] std::string_view const& GetName(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] uint8_t GetArgs(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] uint8_t GetRAArgs(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] FEXCore::IR::RegisterClassType GetRegClass(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] bool HasSideEffects(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] bool ImplicitFlagClobber(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] bool GetHasDest(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] bool LoweredX87(IROps Op);
    [[nodiscard, gnu::const, gnu::visibility("default")]] int8_t TiedSource(IROps Op);

    #undef IROP_SIZES
    #endif
    """))

def print_ir_reg_classes():
    output_file.write("#ifdef IROP_REG_CLASSES_IMPL\n")

    output_file.write("constexpr std::array<FEXCore::IR::RegisterClassType, IROps::OP_LAST + 1> IRRegClasses = {\n")
    for op in IROps:
        if op.Name == "Last":
            output_file.write("\tFEXCore::IR::InvalidClass,\n")
        else:
            Class = "Invalid"
            if op.HasDest and op.DestType == None:
                ExitError("IR op {} has destination with no destination class".format(op.Name))

            if op.HasDest and op.DestType == "SSA": # Special case SSA type
                output_file.write("\tFEXCore::IR::ComplexClass,\n")
            elif op.HasDest:
                output_file.write("\tFEXCore::IR::{}Class,\n".format(op.DestType))
            else:
                # No destination so it has an invalid destination class
                output_file.write("\tFEXCore::IR::InvalidClass, // No destination\n")


    output_file.write("};\n\n")

    output_file.write("// Make sure our array maps directly to the IROps enum\n")
    output_file.write("static_assert(IRRegClasses[IROps::OP_LAST] == FEXCore::IR::InvalidClass);\n\n")

    output_file.write("FEXCore::IR::RegisterClassType GetRegClass(IROps Op) { return IRRegClasses[Op]; }\n\n")

    output_file.write("#undef IROP_REG_CLASSES_IMPL\n")
    output_file.write("#endif\n\n")

# Print out the name printer implementation
def print_ir_getname():
    output_file.write("#ifdef IROP_GETNAME_IMPL\n")
    output_file.write("constexpr std::array<std::string_view const, OP_LAST + 1> IRNames = {\n")
    for op in IROps:
        output_file.write("\t\"{}\",\n".format(op.Name))

    output_file.write("};\n\n")

    output_file.write("static_assert(IRNames[OP_LAST] == \"Last\");\n\n")

    output_file.write("std::string_view const& GetName(IROps Op) {\n")
    output_file.write("  return IRNames[Op];\n")
    output_file.write("}\n")

    output_file.write("#undef IROP_GETNAME_IMPL\n")
    output_file.write("#endif\n\n")

# Print out the number of SSA args that need to be RA'd
def print_ir_getraargs():
    output_file.write("#ifdef IROP_GETRAARGS_IMPL\n")

    output_file.write("constexpr std::array<uint8_t, OP_LAST + 1> IRRAArgs = {\n")
    for op in IROps:
        SSAArgs = op.SSAArgNum

        if op.RAOverride != -1:
            if op.RAOverride > op.SSAArgNum:
                ExitError("Op {} has RA override of {} which is more than total SSA values {}. This doesn't work".format(op.Name, op.RAOverride, op.SSAArgNum))
            SSAArgs = op.RAOverride

        output_file.write("\t{},\n".format(SSAArgs))

    output_file.write("};\n\n")


    output_file.write("constexpr std::array<uint8_t, OP_LAST + 1> IRArgs = {\n")
    for op in IROps:
        SSAArgs = op.SSAArgNum
        output_file.write("\t{},\n".format(SSAArgs))

    output_file.write("};\n\n")

    output_file.write("uint8_t GetRAArgs(IROps Op) {\n")
    output_file.write("  return IRRAArgs[Op];\n")
    output_file.write("}\n")

    output_file.write("uint8_t GetArgs(IROps Op) {\n")
    output_file.write("  return IRArgs[Op];\n")
    output_file.write("}\n")

    output_file.write("#undef IROP_GETRAARGS_IMPL\n")
    output_file.write("#endif\n\n")

def print_ir_hassideeffects():
    output_file.write("#ifdef IROP_HASSIDEEFFECTS_IMPL\n")

    for prop, T in [
        ("HasSideEffects", "bool"),
        ("ImplicitFlagClobber", "bool"),
        ("LoweredX87", "bool"),
        ("TiedSource", "int8_t"),
    ]:
        output_file.write(
            f"constexpr std::array<{'uint8_t' if T == 'bool' else T}, OP_LAST + 1> {prop}_ = {{\n"
        )
        for op in IROps:
            if T == "bool":
                output_file.write(
                    "\t{},\n".format(("true" if getattr(op, prop) else "false"))
                )
            else:
                output_file.write(f"\t{getattr(op, prop)},\n")

        output_file.write("};\n\n")

        output_file.write(f"{T} {prop}(IROps Op) {{\n")
        output_file.write(f"  return {prop}_[Op];\n")
        output_file.write("}\n")

    output_file.write("#undef IROP_HASSIDEEFFECTS_IMPL\n")
    output_file.write("#endif\n\n")

def print_ir_gethasdest():
    output_file.write("#ifdef IROP_GETHASDEST_IMPL\n")

    output_file.write("constexpr std::array<bool, OP_LAST + 1> IRDest = {\n")
    for op in IROps:
        if op.HasDest:
            output_file.write("\ttrue,\n")
        else:
            output_file.write("\tfalse,\n")

    output_file.write("};\n\n")

    output_file.write("bool GetHasDest(IROps Op) {\n")
    output_file.write("  return IRDest[Op];\n")
    output_file.write("}\n")

    output_file.write("#undef IROP_GETHASDEST_IMPL\n")
    output_file.write("#endif\n\n")

# Print out IR argument printing
def print_ir_arg_printer():
    output_file.write("#ifdef IROP_ARGPRINTER_HELPER\n")
    output_file.write("switch (IROp->Op) {\n")
    for op in IROps:
        if not op.ArgPrinter:
            continue

        output_file.write("case IROps::OP_{}: {{\n".format(op.Name.upper()))

        if len(op.Arguments) != 0:
            output_file.write("\t[[maybe_unused]] auto Op = IROp->C<IR::IROp_{}>();\n".format(op.Name))
            output_file.write("\t*out << \" \";\n")

            SSAArgNum = 0
            FirstArg = True
            for i in range(0, len(op.Arguments)):
                arg = op.Arguments[i]

                # No point printing temporaries that we can't recover
                if arg.Temporary:
                    continue

                if FirstArg:
                    FirstArg = False
                else:
                    output_file.write('\t*out << ", ";\n')

                if arg.IsSSA:
                    # SSA value
                    output_file.write("\tPrintArg(out, IR, Op->Header.Args[{}]);\n".format(SSAArgNum))
                    SSAArgNum = SSAArgNum + 1
                else:
                    # User defined op that is stored
                    output_file.write("\tPrintArg(out, IR, Op->{});\n".format(arg.Name))

        output_file.write("break;\n")
        output_file.write("}\n")

    output_file.write("#undef IROP_ARGPRINTER_HELPER\n")
    output_file.write("#endif\n")

def print_validation(op):
    if op.EmitValidation != None:
        output_file.write("\t\t#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED\n")

        for Validation in op.EmitValidation:
            Sanitized = Validation.replace("\"", "\\\"")
            output_file.write("\tLOGMAN_THROW_A_FMT({}, \"{}\");\n".format(Validation, Sanitized))
        output_file.write("\t\t#endif\n")

# Print out IR allocator helpers
def print_ir_allocator_helpers():
    output_file.write("#ifdef IROP_ALLOCATE_HELPERS\n")

    output_file.write("\ttemplate <class T>\n")
    output_file.write("\tstruct Wrapper final {\n")
    output_file.write("\t\tT *first;\n")
    output_file.write("\t\tOrderedNode *Node; ///< Actual offset of this IR in ths list\n")
    output_file.write("\n")
    output_file.write("\t\toperator Wrapper<IROp_Header>() const { return Wrapper<IROp_Header> {reinterpret_cast<IROp_Header*>(first), Node}; }\n")
    output_file.write("\t\toperator OrderedNode *() { return Node; }\n")
    output_file.write("\t\toperator const OrderedNode *() const { return Node; }\n")
    output_file.write("\t\toperator OpNodeWrapper () const { return Node->Header.Value; }\n")
    output_file.write("\t};\n")

    output_file.write("\ttemplate <class T>\n")
    output_file.write("\tusing IRPair = Wrapper<T>;\n\n")

    output_file.write("\tIRPair<IROp_Header> AllocateRawOp(size_t HeaderSize) {\n")
    output_file.write("\t\tauto Op = reinterpret_cast<IROp_Header*>(DualListData.DataAllocate(HeaderSize));\n")
    output_file.write("\t\tmemset(Op, 0, HeaderSize);\n")
    output_file.write("\t\tOp->Op = IROps::OP_DUMMY;\n")
    output_file.write("\t\treturn IRPair<IROp_Header>{Op, CreateNode(Op)};\n")
    output_file.write("\t}\n\n")

    output_file.write("\ttemplate<class T, IROps T2>\n")
    output_file.write("\tT *AllocateOrphanOp() {\n")
    output_file.write("\t\tsize_t Size = FEXCore::IR::GetSize(T2);\n")
    output_file.write("\t\tauto Op = reinterpret_cast<T*>(DualListData.DataAllocate(Size));\n")
    output_file.write("\t\tmemset(Op, 0, Size);\n")
    output_file.write("\t\tOp->Header.Op = T2;\n")
    output_file.write("\t\treturn Op;\n")
    output_file.write("\t}\n\n")

    output_file.write("\ttemplate<class T, IROps T2>\n")
    output_file.write("\tIRPair<T> AllocateOp() {\n")
    output_file.write("\t\tsize_t Size = FEXCore::IR::GetSize(T2);\n")
    output_file.write("\t\tauto Op = reinterpret_cast<T*>(DualListData.DataAllocate(Size));\n")
    output_file.write("\t\tmemset(Op, 0, Size);\n")
    output_file.write("\t\tOp->Header.Op = T2;\n")
    output_file.write("\t\treturn IRPair<T>{Op, CreateNode(&Op->Header)};\n")
    output_file.write("\t}\n\n")

    output_file.write("\tIR::OpSize GetOpSize(const OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(DualListData.DataBegin());\n")
    output_file.write("\t\treturn HeaderOp->Size;\n")
    output_file.write("\t}\n\n")

    output_file.write("\tIR::OpSize GetOpElementSize(const OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(DualListData.DataBegin());\n")
    output_file.write("\t\treturn HeaderOp->ElementSize;\n")
    output_file.write("\t}\n\n")

    output_file.write("\tuint8_t GetOpElements(const OrderedNode *Op) const {\n")
    output_file.write("\t\tLOGMAN_THROW_A_FMT(OpHasDest(Op), \"Op {} has no dest\\n\", GetOpName(Op));\n")
    output_file.write("\t\treturn IR::OpSizeToSize(GetOpSize(Op)) / IR::OpSizeToSize(GetOpElementSize(Op));\n")
    output_file.write("\t}\n\n")

    output_file.write("\tbool OpHasDest(const OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(DualListData.DataBegin());\n")
    output_file.write("\t\treturn GetHasDest(HeaderOp->Op);\n")
    output_file.write("\t}\n\n")

    output_file.write("\tIROps GetOpType(const OrderedNode *Op) const {\n")
    output_file.write("\t\tauto HeaderOp = Op->Header.Value.GetNode(DualListData.DataBegin());\n")
    output_file.write("\t\treturn HeaderOp->Op;\n")
    output_file.write("\t}\n\n")

    output_file.write("\tFEXCore::IR::RegisterClassType GetOpRegClass(const OrderedNode *Op) const {\n")
    output_file.write("\t\treturn GetRegClass(GetOpType(Op));\n")
    output_file.write("\t}\n\n")

    output_file.write("\tstd::string_view const& GetOpName(const OrderedNode *Op) const {\n")
    output_file.write("\t\treturn IR::GetName(GetOpType(Op));\n")
    output_file.write("\t}\n\n")

    # Generate helpers with operands
    for op in IROps:
        if op.Name != "Last":
            output_file.write("\tIRPair<IROp_{}> _{}(" .format(op.Name, op.Name))

            # Output SSA args first
            for i in range(0, len(op.Arguments)):
                arg = op.Arguments[i]
                LastArg = len(op.Arguments) - i - 1 == 0

                if arg.Temporary:
                    CType = IRTypesToCXX[arg.Type].CXXName
                    output_file.write("{} {}".format(CType, arg.Name));
                elif arg.IsSSA:
                    # SSA value
                    output_file.write("OrderedNodeWrapper {}".format(arg.Name))
                else:
                    # User defined op that is stored
                    CType = IRTypesToCXX[arg.Type].CXXName
                    output_file.write("{} {}".format(CType, arg.Name));

                if arg.DefaultInitializer != None:
                    output_file.write(" = {}".format(arg.DefaultInitializer))

                if not LastArg:
                    output_file.write(", ")

            output_file.write(") {\n")

            # Save NZCV if needed before clobbering NZCV
            if op.ImplicitFlagClobber:
                output_file.write("\t\tSaveNZCV(IROps::OP_{});".format(op.Name.upper()))

            # We gather the "has x87?" flag as we go. This saves the user from
            # having to keep track of whether they emitted any x87.
            # Also changes the mmx state to X87.
            if op.LoweredX87:
                output_file.write("\t\tRecordX87Use();\n")
                output_file.write(
                    "\t\tif(MMXState == MMXState_MMX) ChgStateMMX_X87();\n"
                )

            output_file.write("\t\tauto _Op = AllocateOp<IROp_{}, IROps::OP_{}>();\n".format(op.Name, op.Name.upper()))

            if op.SSAArgNum != 0:
                for arg in op.Arguments:
                    if arg.IsSSA:
                        output_file.write("\t\t_Op.first->{} = {};\n".format(arg.Name, arg.Name))

            if len(op.Arguments) != 0:
                for arg in op.Arguments:
                    if not arg.Temporary and not arg.IsSSA:
                        output_file.write("\t\t_Op.first->{} = {};\n".format(arg.Name, arg.Name))

            assert not (op.HasDest and op.DestSize is None)

            # Some ops without a destination still need an operating size
            # Effectively reusing the destination size value for operation size
            if op.DestSize != None:
                output_file.write("\t\t_Op.first->Header.Size = {};\n".format(op.DestSize))

            if op.ElementSize == None:
                output_file.write("\t\t_Op.first->Header.ElementSize = _Op.first->Header.Size;\n")
            else:
                output_file.write("\t\t_Op.first->Header.ElementSize = {};\n".format(op.ElementSize))


            # Only validate here if there's no OrderedNode * version. Else
            # validation is in that version, see the comment below.
            if op.SSAArgNum == 0:
                print_validation(op)

            output_file.write("\t\treturn _Op;\n")
            output_file.write("\t}\n\n")

            # Now do the OrderedNode * version if necessary
            if op.SSAArgNum:
                output_file.write("\tIRPair<IROp_{}> _{}(" .format(op.Name, op.Name))

                for i in range(0, len(op.Arguments)):
                    arg = op.Arguments[i]
                    LastArg = len(op.Arguments) - i - 1 == 0

                    if arg.Temporary:
                        CType = IRTypesToCXX[arg.Type].CXXName
                        output_file.write("{} {}".format(CType, arg.Name));
                    elif arg.IsSSA:
                        output_file.write("OrderedNode *{}".format(arg.Name))
                    else:
                        CType = IRTypesToCXX[arg.Type].CXXName
                        output_file.write("{} {}".format(CType, arg.Name));

                    if arg.DefaultInitializer != None:
                        output_file.write(" = {}".format(arg.DefaultInitializer))

                    if not LastArg:
                        output_file.write(", ")

                output_file.write(") {\n")
                output_file.write("\t\tauto ListDataBegin = DualListData.ListBegin();\n")

                for arg in op.Arguments:
                    if arg.IsSSA:
                        output_file.write("\t\t{}->AddUse();\n".format(arg.Name))

                # Insert validation here. This is skipped for the
                # OrderedNodeWrapper version because validation can depend on
                # the OrderedNode, but that's ok in practice. Everything pre-RA
                # uses the OrderedNode version, and anything RA-onwards is
                # dubious to validate.
                print_validation(op)

                output_file.write(f"\t\treturn _{op.Name}(")
                for i in range(0, len(op.Arguments)):
                    arg = op.Arguments[i]
                    LastArg = len(op.Arguments) - i - 1 == 0
                    output_file.write(arg.Name)
                    if arg.IsSSA:
                        output_file.write("->Wrapped(ListDataBegin)")
                    if not LastArg:
                        output_file.write(", ")
                output_file.write(");\n");
                output_file.write("\t}\n\n");

    output_file.write("#undef IROP_ALLOCATE_HELPERS\n")
    output_file.write("#endif\n")

def print_ir_dispatcher_defs():
    output_dispatch_file.write("#ifdef IROP_DISPATCH_DEFS\n")
    for op in IROps:
        if op.Name != "Last" and op.SwitchGen and op.JITDispatch and op.JITDispatchOverride == None:
            output_dispatch_file.write("DEF_OP({});\n".format(op.Name))

    output_dispatch_file.write("#undef IROP_DISPATCH_DEFS\n")
    output_dispatch_file.write("#endif\n")

def print_ir_dispatcher_dispatch():
    output_dispatch_file.write("#ifdef IROP_DISPATCH_DISPATCH\n")
    for op in IROps:
        if op.Name != "Last" and op.JITDispatch:
            DispatchName = op.Name
            if op.JITDispatchOverride != None:
                DispatchName = op.JITDispatchOverride

            if (op.DynamicDispatch):
                output_dispatch_file.write("REGISTER_OP_RT({}, {});\n".format(op.Name.upper(), DispatchName))
            else:
                output_dispatch_file.write("REGISTER_OP({}, {});\n".format(op.Name.upper(), DispatchName))

    output_dispatch_file.write("#undef IROP_DISPATCH_DISPATCH\n")
    output_dispatch_file.write("#endif\n")


if (len(sys.argv) < 4):
    ExitError()

output_filename = sys.argv[2]
output_dispatcher_filename = sys.argv[3]

json_file = open(sys.argv[1], "r")
json_text = json_file.read()
json_file.close()

json_object = json.loads(json_text)
json_object = {k.upper(): v for k, v in json_object.items()}

ops = json_object["OPS"]
irtypes = json_object["IRTYPES"]
defines = json_object["DEFINES"]

parse_irtypes(irtypes)
parse_ops(ops)

output_file = open(output_filename, "w")

print_enums()
print_ir_structs(defines)
print_ir_sizes()
print_ir_reg_classes()
print_ir_getname()
print_ir_getraargs()
print_ir_hassideeffects()
print_ir_gethasdest()
print_ir_arg_printer()
print_ir_allocator_helpers()

output_file.close()

output_dispatch_file = open(output_dispatcher_filename, "w")
print_ir_dispatcher_defs()
print_ir_dispatcher_dispatch()

output_dispatch_file.close()
