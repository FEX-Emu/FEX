#!/usr/bin/python3
import clang.cindex
from clang.cindex import CursorKind
from clang.cindex import TypeKind
from clang.cindex import TranslationUnit
import sys
from dataclasses import dataclass
import subprocess
import logging
logger = logging.getLogger()
logger.setLevel(logging.WARNING)

# These defines are temporarily defined since python3-clang doesn't yet support these.
# Once this tool gets switched over to C++ then this won't be an issue.
# Type definitions redeclared from `clang/include/clang-c/Index.h`

# Expression that references a C++20 concept.
CursorKind.CONCEPTSPECIALIZATIONEXPR = CursorKind(153),

# Expression that references a C++20 concept.
CursorKind.REQUIRESEXPR = CursorKind(154),

# C++2a std::bit_cast expression.
CursorKind.BUILTINBITCASTEXPR = CursorKind(280)

try:
    # a concept declaration.
    CursorKind.CONCEPTDECL = CursorKind(604),
except:
    pass

@dataclass
class TypeDefinition:
    TYPE_UNKNOWN = 0
    TYPE_STRUCT = 1
    TYPE_UNION = 2
    TYPE_FIELD = 3
    TYPE_VARDECL = 4

    name: str
    type: int
    def __init__(self, Name, Type):
        self.name = Name
        self.type = Type

    @property
    def Name(self):
        return self.name
    @property
    def Type(self):
        return self.type

@dataclass
class AliasType:
    ALIAS_X86_32  = 0
    ALIAS_X86_64  = 1
    ALIAS_AARCH64 = 2
    ALIAS_WIN32   = 3
    ALIAS_WIN64   = 4
    Name: str
    AliasType: int
    def __init__(self, Name, Type):
        self.Name = Name
        self.AliasType = Type

@dataclass
class StructDefinition(TypeDefinition):
    Size: int
    Aliases: list
    Members: list
    ExpectFEXMatch: bool

    def __init__(self, Name, Size):
        super(StructDefinition, self).__init__(Name, TypeDefinition.TYPE_STRUCT)
        self.Size = Size
        self.Aliases = []
        self.Members = []
        self.ExpectFEXMatch = False

@dataclass
class UnionDefinition(TypeDefinition):
    Size: int
    Aliases: list
    Members: list
    ExpectFEXMatch: bool

    def __init__(self, Name, Size):
        super(UnionDefinition, self).__init__(Name, TypeDefinition.TYPE_UNION)
        self.Size = Size
        self.Aliases = []
        self.Members = []
        self.ExpectFEXMatch = False

@dataclass
class FieldDefinition(TypeDefinition):
    Size: int
    OffsetOf: int
    Alignment: int
    def __init__(self, Name, Size, OffsetOf, Alignment):
        super(FieldDefinition, self).__init__(Name, TypeDefinition.TYPE_FIELD)
        self.Size = Size
        self.OffsetOf = OffsetOf
        self.Alignment = Alignment

@dataclass
class VarDeclDefinition(TypeDefinition):
    Size: int
    Aliases: list
    ExpectFEXMatch: bool
    Value: str

    def __init__(self, Name, Size):
        super(VarDeclDefinition, self).__init__(Name, TypeDefinition.TYPE_VARDECL)
        self.Size = Size
        self.Aliases = []
        self.ExpectFEXMatch = False

@dataclass
class ArchDB:
    Parsed: bool
    ArchName: str
    NamespaceScope: list
    CurrentNamespace: str
    TU: TranslationUnit
    Structs: dict
    Unions: dict
    VarDecls: dict
    FieldDecls: list
    def __init__(self, ArchName):
        self.Parsed = True
        self.ArchName = ArchName
        self.NamespaceScope = []
        self.CurrentNamespace = ""
        self.TU = None
        self.Structs = {}
        self.Unions = {}
        self.VarDecls = {}
        self.FieldDecls = []

class DBList:
    DBs: list
    def __init__(self, DB32, DB64, DBAArch64, DBWin32, DBWin64):
        self.DBs = [DB32, DB64, DBAArch64, DBWin32, DBWin64]

def FindClangArguments(OriginalArguments):
    AddedArguments = ["clang"]
    AddedArguments.extend(OriginalArguments)
    AddedArguments.extend(["-v", "-x", "c++", "-S", "-"])
    Proc = subprocess.Popen(AddedArguments, stderr = subprocess.PIPE, stdin = subprocess.DEVNULL)
    NewIncludes = []
    BeginSearch = False
    while True:
        Line = Proc.stderr.readline().strip()

        if not Line:
            Proc.terminate()
            break

        if (Line == b"End of search list."):
            BeginSearch = False
            Proc.terminate()
            break

        if (BeginSearch == True):
            NewIncludes.append("-I" + Line.decode('ascii'))

        if (Line == b"#include <...> search starts here:"):
            BeginSearch = True

    # Add back original arguments
    NewIncludes.extend(OriginalArguments)
    return NewIncludes

def SetNamespace(Arch):
    Arch.CurrentNamespace = ""
    for Namespace in Arch.NamespaceScope:
        Arch.CurrentNamespace = Arch.CurrentNamespace + Namespace + "::"

def HandleStructDeclCursor(Arch, Cursor, NameOverride = ""):
    # Append namespace
    CursorName = ""
    StructType = Cursor.type
    if (len(StructType.spelling) == 0):
        CursorName = NameOverride
    else:
        CursorName = StructType.spelling

    if (len(CursorName) != 0):
        Arch.NamespaceScope.append(CursorName)
        SetNamespace(Arch)

    Struct = StructDefinition(
        Name = CursorName,
        Size = StructType.get_size())

    # Handle children
    Arch.Structs[Struct.Name] = HandleStructElements(Arch, Struct, Cursor)

    # Pop namespace off
    if (len(CursorName) != 0):
        Arch.NamespaceScope.pop()
        SetNamespace(Arch)

    return Arch

def HandleUnionDeclCursor(Arch, Cursor, NameOverride = ""):
    # Append namespace
    CursorName = ""

    if (len(Cursor.spelling) == 0):
        CursorName = NameOverride
    else:
        CursorName = Cursor.spelling

    if (len(CursorName) != 0):
        Arch.NamespaceScope.append(CursorName)
        SetNamespace(Arch)

    UnionType = Cursor.type
    Union = UnionDefinition(
        Name = CursorName,
        Size = UnionType.get_size())
    Arch.Unions[Union.Name] = Union

    # Handle children
    Arch.Unions[Union.Name] = HandleStructElements(Arch, Union, Cursor)

    # Pop namespace off
    if (len(CursorName) != 0):
        Arch.NamespaceScope.pop()
        SetNamespace(Arch)

    return Arch

def HandleVarDeclCursor(Arch, Cursor):
    CursorName = Cursor.spelling
    DeclType = Cursor.type
    Def = Cursor.get_definition()

    VarDecl = VarDeclDefinition(
        Name = CursorName,
        Size = DeclType.get_size())
    Arch.VarDecls[VarDecl.Name] = HandleVarDeclElements(Arch, VarDecl, Cursor)
    return Arch

def HandleVarDeclElements(Arch, VarDecl, Cursor):
    for Child in Cursor.get_children():

        if (Child.kind == CursorKind.ANNOTATE_ATTR):
            if (Child.spelling.startswith("ioctl-alias-")):
                Sections = Child.spelling.split("-")
                if (Sections[2] == "x86_32"):
                    VarDecl.Aliases.append(AliasType(Sections[3], AliasType.ALIAS_X86_32))
                elif (Sections[2] == "x86_64"):
                    VarDecl.Aliases.append(AliasType(Sections[3], AliasType.ALIAS_X86_64))
                elif (Sections[2] == "aarch64"):
                    VarDecl.Aliases.append(AliasType(Sections[3], AliasType.ALIAS_AARCH64))
                elif (Sections[2] == "win32"):
                    VarDecl.Aliases.append(AliasType(Sections[3], AliasType.ALIAS_WIN32))
                elif (Sections[2] == "win64"):
                    VarDecl.Aliases.append(AliasType(Sections[3], AliasType.ALIAS_WIN64))
                else:
                    logging.critical ("Can't handle alias type '{0}'".format(Child.spelling))
                    Arch.Parsed = False
            elif (Child.spelling == "fex-match"):
                VarDecl.ExpectFEXMatch = True
            else:
                # Unknown annotation
                pass
        elif (Child.kind == CursorKind.TYPE_REF or
              Child.kind == CursorKind.UNEXPOSED_EXPR or
              Child.kind == CursorKind.PAREN_EXPR or
              Child.kind == CursorKind.BINARY_OPERATOR
              ):
              pass

    return VarDecl


def HandleTypeDefDeclCursor(Arch, Cursor):
    TypeDefType = Cursor.underlying_typedef_type
    CanonicalType = TypeDefType.get_canonical()

    TypeDefName = Cursor.type.get_typedef_name()

    if (TypeDefType.kind == TypeKind.ELABORATED and CanonicalType.kind == TypeKind.RECORD):
        if (len(TypeDefName) != 0):
            HandleTypeDefDecl(Arch, Cursor, TypeDefName)

            # Append namespace
            Arch.NamespaceScope.append(TypeDefName)
            SetNamespace(Arch)

            Arch = HandleCursor(Arch, Cursor)
            #StructType = Cursor.type
            #Struct = StructDefinition(
            #    Name = TypeDefName,
            #    Size = CanonicalType.get_size())
            #Arch.Structs[TypeDefName] = Struct

            ## Handle children
            #Arch.Structs[TypeDefName] = HandleStructElements(Arch, Struct, Cursor)

            # Pop namespace off
            Arch.NamespaceScope.pop()
            SetNamespace(Arch)
    else:
        if (len(TypeDefName) != 0):
            Def = Cursor.get_definition()

            VarDecl = VarDeclDefinition(
                Name = TypeDefName,
                Size = CanonicalType.get_size())
            Arch.VarDecls[VarDecl.Name] = HandleVarDeclElements(Arch, VarDecl, Cursor)

    return Arch

def HandleStructElements(Arch, Struct, Cursor):
    for Child in Cursor.get_children():
        # logging.info ("\t\tStruct/Union Children: Cursor \"{0}{1}\" of kind {2}".format(Arch.CurrentNamespace, Child.spelling, Child.kind))
        if (Child.kind == CursorKind.ANNOTATE_ATTR):
            if (Child.spelling.startswith("alias-")):
                Sections = Child.spelling.split("-")
                if (Sections[1] == "x86_32"):
                    Struct.Aliases.append(AliasType(Sections[2], AliasType.ALIAS_X86_32))
                elif (Sections[1] == "x86_64"):
                    Struct.Aliases.append(AliasType(Sections[2], AliasType.ALIAS_X86_64))
                elif (Sections[1] == "aarch64"):
                    Struct.Aliases.append(AliasType(Sections[2], AliasType.ALIAS_AARCH64))
                elif (Sections[1] == "win32"):
                    Struct.Aliases.append(AliasType(Sections[2], AliasType.ALIAS_WIN32))
                elif (Sections[1] == "win64"):
                    Struct.Aliases.append(AliasType(Sections[2], AliasType.ALIAS_WIN64))
                else:
                    logging.critical ("Can't handle alias type '{0}'".format(Child.spelling))
                    Arch.Parsed = False

            elif (Child.spelling == "fex-match"):
                Struct.ExpectFEXMatch = True
            else:
                # Unknown annotation
                pass
        elif (Child.kind == CursorKind.FIELD_DECL):
            ParentType = Cursor.type
            FieldType = Child.type
            Field = FieldDefinition(
                Name = Child.spelling,
                Size = FieldType.get_size(),
                OffsetOf = ParentType.get_offset(Child.spelling),
                Alignment = FieldType.get_align())

            #logging.info ("\t{0}".format(Child.spelling))
            #logging.info ("\t\tSize of type: {0}".format(FieldType.get_size()));
            #logging.info ("\t\tAlignment of type: {0}".format(FieldType.get_align()));
            #logging.info ("\t\tOffsetof of type: {0}".format(ParentType.get_offset(Child.spelling)));
            Struct.Members.append(Field)
            Arch.FieldDecls.append(Field)
        elif (Child.kind == CursorKind.STRUCT_DECL):
            ParentType = Cursor.type
            FieldType = Child.type
            Field = FieldDefinition(
                Name = Child.spelling,
                Size = FieldType.get_size(),
                OffsetOf = ParentType.get_offset(Child.spelling),
                Alignment = FieldType.get_align())

            #logging.info ("\t{0}".format(Child.spelling))
            #logging.info ("\t\tSize of type: {0}".format(FieldType.get_size()));
            #logging.info ("\t\tAlignment of type: {0}".format(FieldType.get_align()));
            #logging.info ("\t\tOffsetof of type: {0}".format(ParentType.get_offset(Child.spelling)));
            Struct.Members.append(Field)
            Arch.FieldDecls.append(Field)
            Arch = HandleStructDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.UNION_DECL):
            Struct = HandleStructElements(Arch, Struct, Child)
            #ParentType = Cursor.type
            #FieldType = Child.type
            #Field = FieldDefinition(
            #    Name = Child.spelling,
            #    Size = FieldType.get_size(),
            #    OffsetOf = ParentType.get_offset(Child.spelling),
            #    Alignment = FieldType.get_align())

            #logging.info ("\t{0}".format(Child.spelling))
            #logging.info ("\t\tSize of type: {0}".format(FieldType.get_size()));
            #logging.info ("\t\tAlignment of type: {0}".format(FieldType.get_align()));
            #logging.info ("\t\tOffsetof of type: {0}".format(ParentType.get_offset(Child.spelling)));
            #Struct.Members.append(Field)
            #Arch.FieldDecls.append(Field)
            #Arch = HandleUnionDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.TYPEDEF_DECL):
            Arch = HandleTypeDefDeclCursor(Arch, Child)
        else:
            Arch = HandleCursor(Arch, Child)

    return Struct

def HandleTypeDefDecl(Arch, Cursor, Name):
    for Child in Cursor.get_children():
        if (Child.kind == CursorKind.UNION_DECL):
            pass
        elif (Child.kind == CursorKind.STRUCT_DECL):
            Arch = HandleStructDeclCursor(Arch, Child, Name)
        elif (Child.kind == CursorKind.UNION_DECL):
            Arch = HandleUnionDeclCursor(Arch, Child, Name)
        elif (Child.kind == CursorKind.TYPEDEF_DECL):
            Arch = HandleTypeDefDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.TYPE_REF or
              Child.kind == CursorKind.NAMESPACE_REF or
              Child.kind == CursorKind.TEMPLATE_REF or
              Child.kind == CursorKind.ALIGNED_ATTR):
            # Safe to pass on
            pass
        else:
            logging.critical ("Unhandled TypedefDecl {0}-{1}-{2}".format(Child.kind, Child.type.spelling, Child.spelling))

def HandleCursor(Arch, Cursor):
    if (Cursor.kind.is_invalid()):
        Diags = TU.diagnostics
        for Diag in Diags:
            logging.warning (Diag.format())

        Arch.Parsed = False
        return

    for Child in Cursor.get_children():
        kind = Child.kind
        if (kind == CursorKind.TRANSLATION_UNIT):
            Arch = HandleCursor(Arch, Child)
        elif (kind == CursorKind.FIELD_DECL):
            pass
        elif (kind == CursorKind.UNION_DECL):
            Arch = HandleUnionDeclCursor(Arch, Child)
        elif (kind == CursorKind.STRUCT_DECL):
            Arch = HandleStructDeclCursor(Arch, Child)
        elif (kind == CursorKind.TYPEDEF_DECL):
            Arch = HandleTypeDefDeclCursor(Arch, Child)
        elif (kind == CursorKind.VAR_DECL):
            Arch = HandleVarDeclCursor(Arch, Child)
        elif (kind == CursorKind.NAMESPACE):
            # Append namespace
            Arch.NamespaceScope.append(Child.spelling)
            SetNamespace(Arch)

            # Handle children
            Arch = HandleCursor(Arch, Child)

            # Pop namespace off
            Arch.NamespaceScope.pop()
            SetNamespace(Arch)
        elif (kind == CursorKind.TYPE_REF):
            # Safe to pass on
            pass
        else:
            Arch = HandleCursor(Arch, Child)

    return Arch

def GetDB(Arch, filename, args):
    Index = clang.cindex.Index.create()
    try:
        TU = Index.parse(filename, args=args, options=TranslationUnit.PARSE_INCOMPLETE)
    except TranslationUnitLoadError:
        Arch.Parsed = False
        Diags = TU.diagnostics
        for Diag in Diags:
            logging.warning (Diag.format())

        return

    Arch.TU = TU
    HandleCursor(Arch, TU.cursor)

    # Get diagnostics
    Diags = TU.diagnostics
    if (len(Diags) != 0):
        logging.warning ("Diagnostics from Arch: {0}".format(Arch.ArchName))

    for Diag in Diags:
        logging.warning (Diag.format())

    return Arch

def GetCompar(ComparisonName, DBs):
    if (ComparisonName.lower() == "x86_32"):
        return DBs.DBs[AliasType.ALIAS_X86_32]
    elif (ComparisonName.lower() == "x86_64"):
        return DBs.DBs[AliasType.ALIAS_X86_64]
    elif (ComparisonName.lower() == "win32"):
        return DBs.DBs[AliasType.ALIAS_WIN32]
    elif (ComparisonName.lower() == "win64"):
        return DBs.DBs[AliasType.ALIAS_WIN64]
    elif (ComparisonName.lower() == "aarch64"):
        return DBs.DBs[AliasType.ALIAS_AARCH64]

def PrintMissingMembers(Struct1, Struct2):
    for Member1 in Struct1.Members:
        WasMissing = True
        for Member2 in Struct2.Members:
            if (Member1.Name == Member2.Name):
                WasMissing = False
                break
        if (WasMissing):
            logging.error ("\t'{0}' member '{1}' Doesn't exist in '{2}'".format(Struct1.Name, Member1.Name, Struct2.Name));

def CompareStructs(Struct1, Struct2):
    HadWarning = False
    HadError = False

    # Check if the struct size is a mismatch
    if (Struct1.Size != Struct2.Size):
        logging.warning ("\t#### Warning: Struct size mismatch. {0} != {1}".format(Struct1.Size, Struct2.Size))
        logging.warning ("\t\tMight not be a problem if struct isn't in used inside another struct, or end of object")
        HadWarning = True

    # Check if the number of members differ
    if (len(Struct1.Members) != len(Struct2.Members)):
        logging.error ("@@@@ ERROR: Struct fields mismatch! Number of fields don't match! {0} != {1}".format(len(Struct1.Members), len(Struct2.Members)));
        PrintMissingMembers(Struct1, Struct2)
        PrintMissingMembers(Struct2, Struct1)
        HadError = True
    else:
        # Compare the members themselves
        for StructMemberIndex in range(0, len(Struct1.Members)):
            Member1 = Struct1.Members[StructMemberIndex]
            Member2 = Struct2.Members[StructMemberIndex]
            if (Member1.Type == TypeDefinition.TYPE_FIELD):
                if (Member1.Size != Member2.Size):
                    logging.error ("\t@@@@ ERROR: Member '{0}' mismatch Size! {1} != {2}".format(Member1.Name, Member1.Size, Member2.Size));
                    HadError = True
                if (Member1.OffsetOf != Member2.OffsetOf):
                    logging.error ("\t@@@@ ERROR: Member '{0}' mismatch OffsetOf! {1} != {2}".format(Member1.Name, Member1.OffsetOf, Member2.OffsetOf));
                    HadError = True
                if (Member1.Alignment != Member2.Alignment):
                    logging.error ("\t@@@@ ERROR: Member '{0}' mismatch Alignment! {1} != {2}".format(Member1.Name, Member1.Alignment, Member2.Alignment));
                    logging.error ("\t\tProbably not a problem if offset and size matches");
                    HadWarning = True
            else:
                logging.critical ("Oops, didn't handle member type {0}".format(Member1.Type))
        pass

    return not (HadWarning or HadError)

def CompareAliases(DB, DBs):
    Passed = True
    for StructKey, StructDef in DB.Structs.items():
        if (len(StructKey) == 0):
            # XXX: Oops, shouldn't have anonymous structs
            continue

        if (len(StructDef.Aliases) != 0):
            logging.info ("Comparing Aliases {0}".format(StructDef.Name))

        for Alias in StructDef.Aliases:
            OtherDB = DBs.DBs[Alias.AliasType]
            OtherStruct = OtherDB.Structs.get(Alias.Name)
            if (OtherStruct == None):
                logging.critical ("Couldn't find alias {0} in {1} DB".format(Alias.Name, OtherDB.ArchName))
                Passed = False
                continue

            ThisAlias = CompareStructs(StructDef, OtherStruct)
            if not (ThisAlias):
                logging.error ("Couldn't Alias to Arch {0} successfully".format(OtherDB.ArchName))
            Passed &= ThisAlias

    for VarDeclKey, VarDecl in DB.VarDecls.items():
        if (len(VarDeclKey) == 0):
            # XXX: Oops, shouldn't have anonymous vardecls
            continue


        for Alias in VarDecl.Aliases:
            OtherDB = DBs.DBs[Alias.AliasType]
            OtherAlias = OtherDB.VarDecls.get(Alias.Name)
            if (OtherAlias == None):
                logging.critical ("Couldn't find alias {0} in {1} DB".format(Alias.Name, OtherDB.ArchName))
                Passed = False
                continue

            if (VarDecl.Size != OtherAlias.Size):
                logging.critical("VarDecl: {0}/{1} didn't match {2}/{3}: {4:08X} != {5:08X}".format(VarDeclKey, DB.ArchName, Alias.Name,
                    OtherDB.ArchName,
                    VarDecl.Size, OtherAlias.Size))
                Passed = False
                continue


    return Passed

def CompareCrossArch(DB1, DB2):
    Passed = True
    for StructKey, StructDef in DB1.Structs.items():
        if (len(StructKey) == 0):
            # XXX: Oops, shouldn't have anonymous structs
            continue

        logging.info ("Comparing crossArch {0}".format(StructDef.Name))
        if (StructDef.ExpectFEXMatch):
            Struct2 = DB2.Structs.get(StructDef.Name)
            if (Struct2 == None):
                logging.critical ("Couldn't find Struct {0} in {1} DB".format(StructDef.Name, DB2.ArchName))
                Passed = False
                continue

            Passed &= CompareStructs(StructDef, Struct2)

    return Passed

def main():
    if sys.version_info[0] < 3:
        logging.critical ("Python 3 or a more recent version is required.")

    if (len(sys.argv) < 2):
        print ("usage: %s <options> <Header.hpp> <clang arguments...>" % (sys.argv[0]))
        print ("\t-c1 <Type1>: Base Comparison Type");
        print ("\t-c2 <Type2>: Second Comparison Type");
        print ("\t-win: Parse Windows");
        sys.exit ("\t-no-linux: Do not parse Linux");

    ParseLinux = True
    ParseWindows = False

    Header = ""
    Comparison1 = ""
    Comparison2 = ""
    BaseArgs = []

    StartOfArgs = 0

    # Parse our arguments
    ArgIndex = 1
    while ArgIndex < len(sys.argv):
        Arg = sys.argv[ArgIndex]
        if (Arg == "--"):
            StartOfArgs = ArgIndex + 1
            break;

        if (Arg == "-c1"):
            ArgIndex += 1
            Comparison1 = sys.argv[ArgIndex]
        elif (Arg == "-c2"):
            ArgIndex += 1
            Comparison2 = sys.argv[ArgIndex]
        elif (Arg == "-win"):
            ParseWindows = True
        elif (Arg == "-no-linux"):
           ParseLinux = False
        else:
            Header = Arg
            StartOfArgs = ArgIndex + 1
            break

        # Increment
        ArgIndex += 1

    # Add arguments for clang
    for ArgIndex in range(StartOfArgs, len(sys.argv)):
        BaseArgs.append(sys.argv[ArgIndex])

    args_x86_32 = [
        "-isystem", "/usr/i686-linux-gnu/include",
        "-O2",
        "-m32",
        "--target=i686-linux-unknown",
    ]

    args_x86_64 = [
        "-isystem", "/usr/x86_64-linux-gnu/include",
        "-O2",
        "--target=x86_64-linux-unknown",
        "-D_M_X86_64",
    ]

    args_aarch64 = [
        "-isystem", "/usr/aarch64-linux-gnu/include",
        "-O2",
        "--target=aarch64-linux-unknown",
        "-D_M_ARM_64",
    ]

    args_x86_win32 = [
        "-I/usr/lib/gcc/i686-w64-mingw32/10-win32/include/c++/",
        "-I/usr/lib/gcc/i686-w64-mingw32/10-win32/include/c++/i686-w64-mingw32/",
        "-O2",
        "-m32",
        "--target=i686-pc-win32",
    ]

    args_x86_win64 = [
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++/",
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++/x86_64-w64-mingw32/",
        "-O2",
        "--target=x86_64-pc-win32",
    ]

    # Add all the arguments to the different lists
    args_x86_32.extend(BaseArgs)
    args_x86_64.extend(BaseArgs)
    args_aarch64.extend(BaseArgs)
    args_x86_win32.extend(BaseArgs)
    args_x86_win64.extend(BaseArgs)

    # We need to find the default arguments through clang invocations
    args_x86_32 = FindClangArguments(args_x86_32)
    args_x86_64 = FindClangArguments(args_x86_64)
    args_aarch64 = FindClangArguments(args_aarch64)

    args_x86_win32 = FindClangArguments(args_x86_win32)
    args_x86_win64 = FindClangArguments(args_x86_win64)

    Arch_x86_32 = ArchDB("x86_32")
    Arch_x86_64 = ArchDB("x86_64")

    Arch_aarch64 = ArchDB("aarch64")
    Arch_x86_win32 = ArchDB("win32")
    Arch_x86_win64 = ArchDB("win64")

    if (ParseLinux):
        Arch_x86_32 = GetDB(Arch_x86_32, Header, args_x86_32)
        Arch_x86_64 = GetDB(Arch_x86_64, Header, args_x86_64)
        Arch_aarch64 = GetDB(Arch_aarch64, Header, args_aarch64)

        if not (Arch_x86_32.Parsed):
            logging.critical ("Couldn't parse:{0}".format(Arch_x86_32.ArchName))

        if not (Arch_x86_64.Parsed):
            logging.critical ("Couldn't parse:{0}".format(Arch_x86_64.ArchName))

        if not (Arch_aarch64.Parsed):
            logging.critical ("Couldn't parse:{0}".format(Arch_aarch64.ArchName))

    if (ParseWindows):
        Arch_x86_win32 = GetDB(Arch_x86_win32, Header, args_x86_win32)
        Arch_x86_win64 = GetDB(Arch_x86_win64, Header, args_x86_win64)

        if not (Arch_x86_win32.Parsed):
            logging.critical ("Couldn't parse:{0}".format(Arch_x86_win32.ArchName))

        if not (Arch_x86_win64.Parsed):
            logging.critical ("Couldn't parse:{0}".format(Arch_x86_win64.ArchName))

    DBs = DBList(Arch_x86_32,
        Arch_x86_64,
        Arch_aarch64,
        Arch_x86_win32,
        Arch_x86_win64)

    Result = 0
    if (len(Comparison1) != 0 and len(Comparison2) != 0):
        CompDB1 = GetCompar(Comparison1, DBs)
        CompDB2 = GetCompar(Comparison2, DBs)

        # Now compare across the two compared architectures
        Result = 0 if CompareCrossArch(CompDB1, CompDB2) else 1
    elif (len(Comparison1) != 0):
        CompDB1 = GetCompar(Comparison1, DBs)

        # First compare the aliases to make sure we are matching
        Result = 0 if CompareAliases(CompDB1, DBs) else 1

    if (Result == 1):
        logging.error("Execution environment")
        Args = "[ "
        for Arg in sys.argv:
            Args += Arg + ", "
        Args += " ]"
        logging.error(Args)
        Args = ""
        for Arg in sys.argv:
            Args += "\"" + Arg + "\" "

        logging.error(Args)
    return Result

if __name__ == "__main__":
    # execute only if run as a script
    sys.exit(main())
