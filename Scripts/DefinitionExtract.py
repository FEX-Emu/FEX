#!/usr/bin/python3
import clang.cindex
from clang.cindex import CursorKind
from clang.cindex import TypeKind
from clang.cindex import TranslationUnit
import json
import re
import sys
import tempfile
import os
from dataclasses import dataclass, field
import subprocess
import logging
logger = logging.getLogger()
logger.setLevel(logging.WARNING)

HeaderFile = None
HeaderFileTemp = False

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

@dataclass
class DecorationsDefinition:
    Has64BitDecoration: bool
    Has32BitDecoration: bool
    Decorations: list
    Decorations32Bit: list

    def __init__(self, Has64BitDecoration, Has32BitDecoration, Decorations, Decorations32Bit):
        self.Has64BitDecoration = Has64BitDecoration
        self.Has32BitDecoration = Has32BitDecoration
        self.Decorations = Decorations
        self.Decorations32Bit = Decorations32Bit

@dataclass
class DecorationEnabled:
    TotalEnable: bool
    Has64BitEnabled: bool
    Has32BitEnabled: bool
    def __init__(self, TotalEnable, Has64BitEnabled, Has32BitEnabled):
        self.TotalEnable = TotalEnable
        self.Has64BitEnabled = Has64BitEnabled
        self.Has32BitEnabled = Has32BitEnabled
        if not self.TotalEnable:
            self.Has64BitEnabled = False
            self.Has32BitEnabled = False

@dataclass
class ArgumentDecorationsDefinition:
    Type: str
    Decorations: list

    def __init__(self, Type, Decorations):
        self.Type = Type
        self.Decorations = Decorations

@dataclass
class FunctionArgumentDefinitions:
    @dataclass
    class FunctionArgumentDefinitionData:
        Type: str
        Decorations: DecorationsDefinition
        def __init__(self, Type, Decorations):
            self.Type = Type
            self.Decorations = Decorations

    ArgumentData : dict

    def __init__(self):
        self.ArgumentData = {}

@dataclass
class JSONDefinitionDB:
    # Regex operations
    FunctionAllowList: list
    FunctionDisallowList: list

    StructAllowList: list
    StructDisallowList: list

    Headers: str

    # Function decorations
    FunctionDefaultNamespace: str
    FunctionComments: dict
    FunctionDecorations: dict
    FunctionArgumentDefinitions: dict
    FunctionNamespace: dict
    FunctionEnabled: dict

    # Type decorations
    TypeDefaultNamespace: str
    TypeComments: dict
    TypeDecorations: dict
    TypeNamespace: dict
    TypeEnabled: dict

    AllNamespaces: dict

    def __init__(self, FunctionAllowList, FunctionDisallowList, StructAllowList, StructDisallowList):
        self.FunctionAllowList = FunctionAllowList
        self.FunctionDisallowList = FunctionDisallowList
        self.StructAllowList = StructAllowList
        self.StructDisallowList = StructDisallowList

        self.Headers = ""

        # Functions
        self.FunctionDefaultNamespace = None
        self.FunctionComments = {}
        self.FunctionDecorations = {}
        self.FunctionArgumentDefinitions = {}
        self.FunctionNamespace = {}
        self.FunctionEnabled = {}

        # Types
        self.TypeDefaultNamespace = None
        self.TypeComments = {}
        self.TypeDecorations = {}
        self.TypeNamespace = {}
        self.TypeEnabled = {}

        self.AllNamespaces = {}

    # If any regex matches the disallow list then it isn't allowed through.
    # If any regex matches the allow list, then it gets through.
    # If it matches neither then it isn't allowed through.
    def IsAllowedFunction(self, FunctionName):
        for DisallowFunc in self.FunctionDisallowList:
            if re.search(DisallowFunc, FunctionName) != None:
                return False

        for AllowFunc in self.FunctionAllowList:
            if re.search(AllowFunc, FunctionName) != None:
                return True

        return False

    def IsAllowedStruct(self, StructName):
        for DisallowStruct in self.StructDisallowList:
            if re.search(DisallowStruct, StructName) != None:
                return False

        for AllowStruct in self.StructAllowList:
            if re.search(AllowStruct, StructName) != None:
                return True

        return False

    def FindFunctionComment(self, FunctionName):
        if FunctionName in self.FunctionComments:
            return self.FunctionComments[FunctionName]

        return None

    def FindFunctionEnabled(self, FunctionName):
        if FunctionName in self.FunctionEnabled:
            return self.FunctionEnabled[FunctionName]

        # Not found defaults to enabled
        return DecorationEnabled(True, True, True)

    def FindFunctionDecoration(self, FunctionName):
        if FunctionName in self.FunctionDecorations:
            return self.FunctionDecorations[FunctionName]

        return None

    def FindFunctionArgumentDefinitions(self, FunctionName):
        if FunctionName in self.FunctionArgumentDefinitions:
            return self.FunctionArgumentDefinitions[FunctionName]

        return None

    def FindFunctionNamespace(self, FunctionName):
        if FunctionName in self.FunctionNamespace:
            return self.FunctionNamespace[FunctionName]

        return self.FunctionDefaultNamespace


    def FindTypeComment(self, TypeName):
        if TypeName in self.TypeComments:
            return self.TypeComments[TypeName]

        return None

    def FindTypeEnabled(self, TypeName):
        if TypeName in self.TypeEnabled:
            return self.TypeEnabled[TypeName]

        # Not found defaults to enabled
        return DecorationEnabled(True, True, True)

    def FindTypeDecoration(self, TypeName):
        if TypeName in self.TypeDecorations:
            return self.TypeDecorations[TypeName]

        return None

    def FindTypeNamespace(self, TypeName):
        if TypeName in self.TypeNamespace:
            return self.TypeNamespace[TypeName]

        return self.TypeDefaultNamespace

@dataclass
class FunctionDecl:
    Name: str
    Ret: str
    Params: list

    def __init__(self, Name, Ret):
        self.Name = Name
        self.Ret = Ret
        self.Params = []

FunctionDecls = []
StructDecls = []

JSONDefinition = None

ListOfDisallowedFunctions = {}
ListOfDisallowedStructs = {}
def HandleFunctionDeclCursor(Arch, Cursor):
    global ListOfDisallowedFunctions
    if (Cursor.is_definition()):
        return Arch

    #logging.critical ("Unhandled FunctionDeclCursor {0}-{1}-{2}-{3}".format(Cursor.kind, Cursor.type.spelling, Cursor.spelling,
    #    Cursor.result_type.spelling))
    if not JSONDefinition.IsAllowedFunction(Cursor.spelling):
        ListOfDisallowedFunctions[Cursor.spelling] = True
        return Arch

    Function = FunctionDecl(Cursor.spelling, Cursor.result_type.spelling)

    for Child in Cursor.get_children():
        if (Child.kind == CursorKind.TYPE_REF):
            # This will give us the return type
            # We skip this since we get it at the start instead
            pass
        elif (Child.kind == CursorKind.PARM_DECL):
            # This gives us a parameter type
            Function.Params.append(Child.type.spelling)
        elif (Child.kind == CursorKind.ASM_LABEL_ATTR):
            # Whatever you are we don't care about you
            return Arch
        elif (Child.kind == CursorKind.WARN_UNUSED_RESULT_ATTR):
            # Whatever you are we don't care about you
            return Arch
        elif (Child.kind == CursorKind.VISIBILITY_ATTR or
              Child.kind == CursorKind.UNEXPOSED_ATTR or
              Child.kind == CursorKind.CONST_ATTR or
              Child.kind == CursorKind.PURE_ATTR):
            pass
        else:
            logging.critical ("\tUnhandled FunctionDeclCursor {0}-{1}-{2}".format(Child.kind, Child.type.spelling, Child.spelling))
            sys.exit(-1)

    FunctionDecls.append(Function)
    return Arch

def PrintComment(Comment):
    if Comment == None or len(Comment) == 0:
        return

    for Line in Comment:
        print(Line)

def PrintFunctionDecl(Decl):
    logging.critical(Decl.Name)

    Namespace = JSONDefinition.FindFunctionNamespace(Decl.Name)
    Comments = JSONDefinition.FindFunctionComment(Decl.Name)
    Enabled = JSONDefinition.FindFunctionEnabled(Decl.Name)
    Decorations = JSONDefinition.FindFunctionDecoration(Decl.Name)
    ArgumentDecorations = JSONDefinition.FindFunctionArgumentDefinitions(Decl.Name)

    Has32BitDecoration = False
    Has64BitDecoration = False
    Decorations64Bit = ""
    Decorations32Bit = ""
    if Decorations != None:
        logging.critical(Decorations)
        if Decorations.Has32BitDecoration:
            Has32BitDecoration = True
            if len(Decorations.Decorations32Bit):
                Decorations32Bit = " : {}".format(", ".join(Decorations.Decorations32Bit))

        if Decorations.Has64BitDecoration:
            Has64BitDecoration = True
            if len(Decorations.Decorations):
                Decorations64Bit = " : {}".format(", ".join(Decorations.Decorations))

        if len(Decorations.Decorations):
            Decorations64Bit = " : {}".format(", ".join(Decorations.Decorations))

    if Has32BitDecoration:
        print("#ifdef IS_32BIT_THUNK")
        PrintComment(Comments)
        FrontLineComment = ""
        if not Enabled.Has32BitEnabled:
            FrontLineComment = "//"
            print("// TODO: Disabled by order of the JSON.")

        print("{}template<>\n{}struct fex_gen_config<{}>{} {{}};".format(FrontLineComment, FrontLineComment, Decl.Name, Decorations32Bit))
        if not Has64BitDecoration:
            print("#else")
            print("template<>\nstruct fex_gen_config<{}> {{}};".format(Decl.Name))
        print("#endif")

    if Has64BitDecoration:
        print("#ifndef IS_32BIT_THUNK")
        PrintComment(Comments)

        FrontLineComment = ""
        if not Enabled.Has64BitEnabled:
            FrontLineComment = "//"
            print("// TODO: Disabled by order of the JSON.")

        print("{}template<>\n{}struct fex_gen_config<{}>{} {{}};".format(FrontLineComment, FrontLineComment, Decl.Name, Decorations64Bit))
        if not Has32BitDecoration:
            print("#else")
            print("template<>\nstruct fex_gen_config<{}> {{}};".format(Decl.Name))
        print("#endif")

    if not (Has32BitDecoration or Has64BitDecoration):
        PrintComment(Comments)
        FrontLineComment = ""
        if not Enabled.TotalEnable:
            FrontLineComment = "//"
            print("// TODO: Disabled by order of the JSON.")

        print("{}template<>\n{}struct fex_gen_config<{}>{} {{}};".format(FrontLineComment, FrontLineComment, Decl.Name, Decorations64Bit))

    if ArgumentDecorations != None:
        if ArgumentDecorations[1] != None and len(ArgumentDecorations[1]):
            print("#ifndef IS_32BIT_THUNK")
            for DecoNum, DecoData in ArgumentDecorations[1].items():
                Type = ""
                if DecoData.Type != None:
                    Type = ", {}".format(DecoData.Type)
                Decorations = " : {}".format(", ".join(DecoData.Decorations))

                print("  template<>\n  struct fex_gen_param<{}, {}{}>{} {{}};".format(Decl.Name, DecoNum, Type, Decorations))
            print("#endif")

        if ArgumentDecorations[2] != None and len(ArgumentDecorations[2]):
            print("#ifdef IS_32BIT_THUNK")
            for DecoNum, DecoData in ArgumentDecorations[2].items():
                Type = ""
                if DecoData.Type != None:
                    Type = ", {}".format(DecoData.Type)
                Decorations = " : {}".format(", ".join(DecoData.Decorations))

                print("  template<>\n  struct fex_gen_param<{}, {}{}>{} {{}};".format(Decl.Name, DecoNum, Type, Decorations))
            print("#endif")

        if ArgumentDecorations[0] != None and len(ArgumentDecorations[0]):
            for DecoNum, DecoData in ArgumentDecorations[0].items():
                Type = ""
                if DecoData.Type != None:
                    Type = ", {}".format(DecoData.Type)
                Decorations = " : {}".format(", ".join(DecoData.Decorations))

                print("  template<>\n  struct fex_gen_param<{}, {}{}>{} {{}};".format(Decl.Name, DecoNum, Type, Decorations))

def PrintStructDecl(Decl):

    # template<>
    # struct fex_gen_type<VkCommandBuffer_T> : fexgen::opaque_type {};
    logging.critical(Decl.Name)

    Namespace = JSONDefinition.FindTypeNamespace(Decl.Name)
    Comments = JSONDefinition.FindTypeComment(Decl.Name)
    Enabled = JSONDefinition.FindTypeEnabled(Decl.Name)
    Decorations = JSONDefinition.FindTypeDecoration(Decl.Name)

    Has32BitDecoration = False
    Has64BitDecoration = False
    Decorations64Bit = ""
    Decorations32Bit = ""
    if Decorations != None:
        logging.critical(Decorations)
        if Decorations.Has32BitDecoration:
            Has32BitDecoration = True
            if len(Decorations.Decorations32Bit):
                Decorations32Bit = " : {}".format(", ".join(Decorations.Decorations32Bit))

        if Decorations.Has64BitDecoration:
            Has64BitDecoration = True
            if len(Decorations.Decorations):
                Decorations64Bit = " : {}".format(", ".join(Decorations.Decorations))

        if len(Decorations.Decorations):
            Decorations64Bit = " : {}".format(", ".join(Decorations.Decorations))

    if Has32BitDecoration:
        print("#ifdef IS_32BIT_THUNK")
        PrintComment(Comments)
        FrontLineComment = ""
        if not Enabled.Has32BitEnabled:
            FrontLineComment = "//"
            print("// TODO: Disabled by order of the JSON.")

        print("{}template<>\n{}struct fex_gen_type<{}>{} {{}};".format(FrontLineComment, FrontLineComment, Decl.Name, Decorations32Bit))
        if not Has64BitDecoration:
            print("#else")
            print("template<>\nstruct fex_gen_type<{}> {{}};".format(Decl.Name))
        print("#endif")

    if Has64BitDecoration:
        print("#ifndef IS_32BIT_THUNK")
        PrintComment(Comments)

        FrontLineComment = ""
        if not Enabled.Has64BitEnabled:
            FrontLineComment = "//"
            print("// TODO: Disabled by order of the JSON.")

        print("{}template<>\n{}struct fex_gen_type<{}>{} {{}};".format(FrontLineComment, FrontLineComment, Decl.Name, Decorations64Bit))
        if not Has32BitDecoration:
            print("#else")
            print("template<>\nstruct fex_gen_type<{}> {{}};".format(Decl.Name))
        print("#endif")

    if not (Has32BitDecoration or Has64BitDecoration):
        PrintComment(Comments)
        FrontLineComment = ""
        if not Enabled.TotalEnable:
            FrontLineComment = "//"
            print("// TODO: Disabled by order of the JSON.")

        print("{}template<>\n{}struct fex_gen_type<{}>{} {{}};".format(FrontLineComment, FrontLineComment, Decl.Name, Decorations64Bit))

def PrintHeaders():
    for Line in JSONDefinition.Headers:
        print(Line)

def PrintStructDecls():
    CurrentNamespace = None
    PrintedDefinitions = {}

    for Decl in StructDecls:
        if Decl.Name in PrintedDefinitions:
            continue

        PrintStructDecl(Decl)
        PrintedDefinitions[Decl.Name] = True

def PrintFunctionDecls():
    CurrentNamespace = None
    PrintedDefinitions = {}

    # First print all non-namespaced functions, then namespaced functions.
    for Decl in FunctionDecls:
        if Decl.Name in PrintedDefinitions:
            continue

        Namespace = JSONDefinition.FindFunctionNamespace(Decl.Name)
        if Namespace != "":
            continue

        PrintFunctionDecl(Decl)

        PrintedDefinitions[Decl.Name] = True

    for MatchedNamespace in JSONDefinition.AllNamespaces:
        if len(MatchedNamespace):
            print("namespace {} {{".format(MatchedNamespace))
        for Decl in FunctionDecls:
            if Decl.Name in PrintedDefinitions:
                continue

            Namespace = JSONDefinition.FindFunctionNamespace(Decl.Name)
            if Namespace != MatchedNamespace:
                continue

            PrintFunctionDecl(Decl)

            PrintedDefinitions[Decl.Name] = True

        if len(MatchedNamespace):
            print("}} // namespace {}".format(MatchedNamespace))

    # Now print everything else
    for Decl in FunctionDecls:
        if Decl.Name in PrintedDefinitions:
            continue

        PrintFunctionDecl(Decl)

        PrintedDefinitions[Decl.Name] = True

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

    StructName = CursorName.removeprefix("struct ")

    if (len(StructName) != 0):
        Arch.NamespaceScope.append(StructName)
        SetNamespace(Arch)

    Struct = StructDefinition(
        Name = StructName,
        Size = StructType.get_size())

    if not JSONDefinition.IsAllowedStruct(Struct.Name):
        ListOfDisallowedStructs[Struct.Name] = True
        return Arch
    else:
        # Handle children
        Arch.Structs[Struct.Name] = HandleStructElements(Arch, Struct, Cursor, False)
        StructDecls.append(Arch.Structs[Struct.Name])

    # Pop namespace off
    if (len(StructName) != 0):
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

    UnionName = CursorName.removeprefix("union ")

    if (len(UnionName) != 0):
        Arch.NamespaceScope.append(UnionName)
        SetNamespace(Arch)

    UnionType = Cursor.type
    Union = UnionDefinition(
        Name = UnionName,
        Size = UnionType.get_size())
    Arch.Unions[Union.Name] = Union

    if not JSONDefinition.IsAllowedStruct(Union.Name):
        ListOfDisallowedStructs[Union.Name] = True
        return Arch
    else:
        logging.critical("HandleUnionDeclCursor: {}".format(Union.Name))
        # Handle children
        Arch.Unions[Union.Name] = HandleStructElements(Arch, Union, Cursor, False)
        StructDecls.append(Arch.Unions[Union.Name])

    # Handle children
    Arch.Unions[Union.Name] = HandleStructElements(Arch, Union, Cursor, False)

    # Pop namespace off
    if (len(UnionName) != 0):
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
                VarDecl.ExpectedFEXMatch = True
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
            #Arch.Structs[TypeDefName] = HandleStructElements(Arch, Struct, Cursor, False)

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

def HandleStructElements(Arch, Struct, Cursor, Print):
    for Child in Cursor.get_children():
        # logging.critical ("\t\tStruct/Union Children: Cursor \"{0}\" of kind {1}".format(Child.spelling, Child.kind))
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
                Struct.ExpectedFEXMatch = True
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

            if Print:
                logging.info ("\t{0}".format(Child.spelling))
                logging.info ("\t\tSize of type: {0}".format(FieldType.get_size()));
                logging.info ("\t\tAlignment of type: {0}".format(FieldType.get_align()));
                logging.info ("\t\tOffsetof of type: {0}".format(ParentType.get_offset(Child.spelling)));
            Struct.Members.append(Field)
            Arch.FieldDecls.append(Field)
        elif (Child.kind == CursorKind.STRUCT_DECL) or (Child.kind == CursorKind.UNION_DECL):
            ParentType = Cursor.type
            FieldType = Child.type
            Field = FieldDefinition(
                Name = Child.spelling,
                Size = FieldType.get_size(),
                OffsetOf = ParentType.get_offset(Child.spelling),
                Alignment = FieldType.get_align())

            if Print:
                logging.info ("\t{0}".format(Child.spelling))
                logging.info ("\t\tSize of type: {0}".format(FieldType.get_size()));
                logging.info ("\t\tAlignment of type: {0}".format(FieldType.get_align()));
                logging.info ("\t\tOffsetof of type: {0}".format(ParentType.get_offset(Child.spelling)));
            Struct.Members.append(Field)
            Arch.FieldDecls.append(Field)

            Arch = HandleStructDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.TYPEDEF_DECL):
            Arch = HandleTypeDefDeclCursor(Arch, Child)
        else:
            Arch = HandleCursor(Arch, Child)

    return Struct

def HandleTypeDefDecl(Arch, Cursor, Name):
    for Child in Cursor.get_children():
        if (Child.kind == CursorKind.STRUCT_DECL):
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
        if (Child.kind == CursorKind.TRANSLATION_UNIT):
            Arch = HandleCursor(Arch, Child)
        elif (Child.kind == CursorKind.FIELD_DECL):
            pass
        elif (Child.kind == CursorKind.UNION_DECL):
            Arch = HandleUnionDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.STRUCT_DECL):
            Arch = HandleStructDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.TYPEDEF_DECL):
            Arch = HandleTypeDefDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.VAR_DECL):
            Arch = HandleVarDeclCursor(Arch, Child)
        elif (Child.kind == CursorKind.NAMESPACE):
            # Append namespace
            Arch.NamespaceScope.append(Child.spelling)
            SetNamespace(Arch)

            # Handle children
            Arch = HandleCursor(Arch, Child)

            # Pop namespace off
            Arch.NamespaceScope.pop()
            SetNamespace(Arch)
        elif (Child.kind == CursorKind.TYPE_REF):
            # Safe to pass on
            pass
        elif (Child.kind == CursorKind.FUNCTION_DECL):
            # For function printing
            Arch = HandleFunctionDeclCursor(Arch, Child)
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
    FunctionDecls.clear()
    HandleCursor(Arch, TU.cursor)

    # Get diagnostics
    Diags = TU.diagnostics
    if (len(Diags) != 0):
        logging.warning ("Diagnostics from Arch: {0}".format(Arch.ArchName))

    for Diag in Diags:
        logging.warning (Diag.format())

    return Arch

def ParseFunctionComments(Function, DecorationData):
    if "Comment" in DecorationData:
        return DecorationData["Comment"]
    return None

def ParseFunctionDecorations(Function, DecorationData):
    Has32BitDecoration = False
    Has64BitDecoration = False
    Decorations64Bit = []
    Decorations32Bit = []

    if "64Bit" in DecorationData:
        Has64BitDecoration = True
        if "Decorations" in DecorationData["64Bit"]:
            Decorations64Bit = DecorationData["64Bit"]["Decorations"]

    if "32Bit" in DecorationData:
        Has32BitDecoration = True
        if "Decorations" in DecorationData["32Bit"]:
            Decorations32Bit = DecorationData["32Bit"]["Decorations"]
            logging.critical("32bit deco: {} {}".format(Function, Decorations32Bit))

    if "Decorations" in DecorationData:
        Decorations64Bit = DecorationData["Decorations"]
        if Has32BitDecoration or Has64BitDecoration:
            logging.critical("Had base Decorations declared but also 32-bit or 64-bit for {}".format(Function))

    return DecorationsDefinition(Has64BitDecoration, Has32BitDecoration, Decorations64Bit, Decorations32Bit)

def ParseFunctionEnabled(Function, DecorationData):
    TotalEnable = True
    Has32BitEnabled = True
    Has64BitEnabled = True
    if "32Bit" in DecorationData:
        if "Enabled" in DecorationData["32Bit"]:
            Has32BitEnabled = DecorationData["32Bit"]["Enabled"]

    if "64Bit" in DecorationData:
        if "Enabled" in DecorationData["64Bit"]:
            Has64BitEnabled = DecorationData["64Bit"]["Enabled"]

    if "Enabled" in DecorationData:
        TotalEnable = DecorationData["Enabled"]
    return DecorationEnabled(TotalEnable, Has64BitEnabled, Has32BitEnabled)

def DecodeArgumentDecorations(DecorationData):
    ArgumentDefs = {}
    for Parameter, ParameterData in DecorationData.items():
        Type = None
        Decorations = None

        if "Type" in ParameterData:
            Type = ParameterData["Type"]

        if not "Decorations" in ParameterData:
            logging.critical("Function parameter needs decoration if declared")

        ArgumentDefs[Parameter] = ArgumentDecorationsDefinition(Type, ParameterData["Decorations"])
    return ArgumentDefs

def ParseArgumentDefinitions(DecorationData):
    # Parameter decorations are a bit silly. They can be inside "32Bit", "64Bit" or base.

    ArgumentDefs = {}
    ArgumentDefs32Bit = {}
    ArgumentDefs64Bit = {}

    Has32BitDecoration = False
    Has64BitDecoration = False
    Decorations64Bit = []
    Decorations32Bit = []

    if "64Bit" in DecorationData:
        Has64BitDecoration = True
        if "Parameters" in DecorationData["64Bit"]:
            ArgumentDefs64Bit = DecodeArgumentDecorations(DecorationData["64Bit"]["Parameters"])

    if "32Bit" in DecorationData:
        Has32BitDecoration = True
        if "Parameters" in DecorationData["32Bit"]:
            ArgumentDefs32Bit = DecodeArgumentDecorations(DecorationData["32Bit"]["Parameters"])

    if "Parameters" in DecorationData:
        ArgumentDefs = DecodeArgumentDecorations(DecorationData["Parameters"])

    # Validate combination of arguments
    # Any argument declared in 32Bit or 64Bit can't be declared in base.
    for ArgumentDefName, ArgumentDefData in ArgumentDefs32Bit.items():
        if ArgumentDefName in ArgumentDefs:
            logging.critical("{} accidentally declared in base non-bitness parameter def!".format(ArgumentDefName))

    for ArgumentDefName, ArgumentDefData in ArgumentDefs64Bit.items():
        if ArgumentDefName in ArgumentDefs:
            logging.critical("{} accidentally declared in base non-bitness parameter def!".format(ArgumentDefName))

    return (ArgumentDefs, ArgumentDefs64Bit, ArgumentDefs32Bit)

def ParseJSONType(Type, TypeData):
    global JSONDefinition
    logging.critical(Type)
    logging.critical(TypeData)

    if Type in JSONDefinition.TypeDecorations:
        logging.critical("Duplicate function '{}' in json definition!".format(Type))

    JSONDefinition.TypeComments[Type] = ParseFunctionComments(Type, TypeData)
    JSONDefinition.TypeDecorations[Type] = ParseFunctionDecorations(Type, TypeData)
    JSONDefinition.TypeEnabled[Type] = ParseFunctionEnabled(Type, TypeData)
    #logging.critical(JSONDefinition.TypeComments[Type])
    #logging.critical(JSONDefinition.TypeDecorations[Type])
    #logging.critical(JSONDefinition.TypeEnabled[Type])

def ParseJSONFunction(Function, FunctionData):
    global JSONDefinition
    logging.critical(Function)
    logging.critical(FunctionData)

    if Function in JSONDefinition.FunctionDecorations:
        logging.critical("Duplicate function '{}' in json definition!".format(Function))

    JSONDefinition.FunctionComments[Function] = ParseFunctionComments(Function, FunctionData)
    JSONDefinition.FunctionDecorations[Function] = ParseFunctionDecorations(Function, FunctionData)
    JSONDefinition.FunctionArgumentDefinitions[Function] = ParseArgumentDefinitions(FunctionData)
    JSONDefinition.FunctionEnabled[Function] = ParseFunctionEnabled(Function, FunctionData)

    if "Namespace" in FunctionData:
        JSONDefinition.FunctionNamespace[Function] = FunctionData["Namespace"]
        JSONDefinition.AllNamespaces[FunctionData["Namespace"]] = True

def ParseJSONDefinition(filename):
    global JSONDefinition
    global HeaderFile
    global HeaderFileTemp

    # Default json definition
    JSONText = '''
{
  "FunctionAllowList": [
    ".*"
  ]
}
'''
    if len(filename):
        JSONFile = open(filename, "r")
        JSONText = JSONFile.read()
        JSONFile.close()

    json_object = json.loads(JSONText)

    AllowList = [".*"]
    DisallowList = []

    StructAllowList = [".*"]
    StructDisallowList = []

    if "FunctionAllowList" in json_object:
        AllowList = json_object["FunctionAllowList"]

    if "FunctionDisallowList" in json_object:
        DisallowList = json_object["FunctionDisallowList"]

    if "StructAllowList" in json_object:
        StructAllowList = json_object["StructAllowList"]

    if "StructDisallowList" in json_object:
        StructDisallowList = json_object["StructDisallowList"]

    JSONDefinition = JSONDefinitionDB(AllowList, DisallowList, StructAllowList, StructDisallowList)

    if "Headers" in json_object:
        JSONDefinition.Headers = json_object["Headers"]

    if "FunctionDefaultNamespace" in json_object:
        JSONDefinition.FunctionDefaultNamespace = json_object["FunctionDefaultNamespace"]
        JSONDefinition.AllNamespaces[JSONDefinition.FunctionDefaultNamespace] = True

    if "Functions" in json_object:
        for Function, FunctionData  in json_object["Functions"].items():
            ParseJSONFunction(Function, FunctionData)

    if "Types" in json_object:
        for Type, TypeData  in json_object["Types"].items():
            ParseJSONType(Type, TypeData)
    if "GenerationFile" in json_object and HeaderFile == None:
        with tempfile.NamedTemporaryFile(mode='w', delete=False, delete_on_close=False, suffix=".h") as Temp:
            for Line in json_object["GenerationFile"]:
                Temp.write("{}\n".format(Line))
            HeaderFile = Temp.name
            HeaderFileTemp = True
            Temp.close()

def PrintDisallowedFunctions():
    global ListOfDisallowedFunctions
    for Func in ListOfDisallowedFunctions:
        logging.critical("DisallowedFunc: {}".format(Func))

def main():
    global HeaderFile
    global HeaderFileTemp
    if sys.version_info[0] < 3:
        logging.critical ("Python 3 or a more recent version is required.")

    if (len(sys.argv) < 2):
        print ("usage: %s [--json_def <def.json>] [<Header.hpp>] -- <clang arguments...>" % (sys.argv[0]))

    JSONDef = ""
    BaseArgs = []

    BeginClangArgs = 1

    while BeginClangArgs < len(sys.argv):
        Arg = sys.argv[BeginClangArgs]
        if Arg == "--json_def":
            BeginClangArgs += 1
            JSONDef = sys.argv[BeginClangArgs]
            BeginClangArgs += 1
            continue

        if Arg == "--":
            BeginClangArgs += 1
            break
        HeaderFile = sys.argv[BeginClangArgs]
        BeginClangArgs += 1

    # Add arguments for clang
    for ArgIndex in range(BeginClangArgs, len(sys.argv)):
        BaseArgs.append(sys.argv[ArgIndex])

    args_x86_64 = [
        "-isystem", "/usr/include/x86_64-linux-gnu",
        "-isystem", "/usr/x86_64-linux-gnu/include/c++/10/x86_64-linux-gnu/",
        "-isystem", "/usr/x86_64-linux-gnu/include/",
        "-O2",
        "--target=x86_64-linux-unknown",
        "-D_M_X86_64",
    ]

    ParseJSONDefinition(JSONDef)

    # Add all the arguments to the different lists
    args_x86_64.extend(BaseArgs)

    # We need to find the default arguments through clang invocations
    args_x86_64 = FindClangArguments(args_x86_64)

    Arch_x86_64 = ArchDB("x86_64")
    Arch_x86_64 = GetDB(Arch_x86_64, HeaderFile, args_x86_64)

    PrintHeaders()
    PrintStructDecls()
    PrintFunctionDecls()

    PrintDisallowedFunctions()

    if HeaderFileTemp:
        os.remove(HeaderFile)

if __name__ == "__main__":
# execute only if run as a script
    sys.exit(main())
