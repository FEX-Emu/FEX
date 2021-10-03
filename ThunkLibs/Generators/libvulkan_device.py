#!/usr/bin/python3
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from ThunkHelpers import *
import sys

# Skip functions that we need to hand generate
SkipGenFunctions = {
    # We need to handle these manually
    "vkGetDeviceProcAddr",
    "vkGetInstanceProcAddr",
    "vkCreateShaderModule",
    "vkCreateInstance",
    "vkCreateDevice",
    "vkAllocateMemory",
    "vkFreeMemory",

    # Causing thunk problems
    "vkCmdSetFragmentShadingRateKHR",
    "vkCmdSetFragmentShadingRateEnumNV",
}

SkipExtensionFunctions = {
    # Android things
    "VK_ANDROID_external_memory_android_hardware_buffer",
    "VK_ANDROID_native_buffer",
    "VK_KHR_android_surface",
    # MacOS/IOS things
    "VK_EXT_metal_surface",
    "VK_MVK_macos_surface",
    "VK_MVK_ios_surface",
    # Windows things
    "VK_KHR_win32_surface",
    "VK_KHR_external_memory_win32",
    "VK_KHR_external_fence_win32",
    "VK_KHR_external_semaphore_win32",
    "VK_NV_external_memory_win32",
    # Fuchsia things
    "VK_FUCHSIA_external_memory",
    "VK_FUCHSIA_external_semaphore",
    "VK_FUCHSIA_imagepipe_surface",
    "VK_FUCHSIA_buffer_collection",
    # Various random OS
    "VK_QNX_screen_surface",
    "VK_GGP_stream_descriptor_surface",
    "VK_GGP_frame_token",
    "VK_NN_vi_surface",
    # DirectFB support
    "VK_EXT_directfb_surface",
    # Requires vk_video
    "VK_EXT_video_decode_h264",
    "VK_EXT_video_encode_h264",
    "VK_EXT_video_decode_h265",
    "VK_EXT_video_encode_h265",
}

@dataclass
class FunctionDef:
    Ret : str
    Name: str
    Params: str
    Extensions: dict
    def __init__(self, Ret, Name, Params):
        self.Ret = Ret
        self.Name = Name
        self.Params = Params
        self.Extensions = {}

@dataclass
class StructMember:
    Type: str
    Name: str
    Str:  str
    def __init__(self, Type, Name, Str):
        self.Type = Type
        self.Name = Name
        self.Str = Str

@dataclass
class StructDef:
    Type: str
    Name: str
    Emitted: bool
    Members: list
    Requires: list
    def __init__(self, Type, Name):
        self.Type = Type
        self.Name = Name;
        self.Emitted = False
        self.Members = []
        self.Requires = []

@dataclass
class EnumDef:
    Type: str
    Name: str
    Members: list
    Aliases: list
    def __init__(self, Type, Name):
        self.Type = Type
        self.Name = Name
        self.Members = []
        self.Aliases = []
@dataclass
class BaseTypeDef:
    Type: str
    Name: str
    Str: str
    Requires: list
    Emitted: bool
    def __init__(self, Type, Name, Str):
        self.Type = Type
        self.Name = Name
        self.Str = Str
        self.Requires = []
        self.Emitted = False

BaseTypeDefs = {}
DefineDefs = []
EnumDefs = {}
StructDefs = {}
FunctionDefs = {}

# Some additional dependency tracking here
# Hard to get correct ordering otherwise
AdditionalStructDefines = {
    'VkDebugUtilsMessengerCreateInfoEXT': 'PFN_vkDebugUtilsMessengerCallbackEXT',
    'VkDeviceDeviceMemoryReportCreateInfoEXT': 'PFN_vkDeviceMemoryReportCallbackEXT',
}

def LoadVulkanXML(Filename):
    return ET.parse(Filename)

# Walks the enums element in the XML and pulls them all out
def parse_enums(root):
    for enum in root.findall('enums'):
        if 'type' in enum.attrib:
            Type = enum.attrib['type']
            Name = enum.attrib['name']
            if Type == 'enum':
                # Actually an enum
                Def = EnumDef(Type, Name)
                for value in enum.findall('enum'):
                    if 'alias' in value.attrib:
                        Def.Members.append("{0} = {1},".format(value.attrib['name'], value.attrib['alias']))
                    else:
                        Def.Members.append("{0} = {1},".format(value.attrib['name'], value.attrib['value']))
                EnumDefs[Name] = Def
            elif Type == 'bitmask':
                Def = EnumDef("enum", Name)
                for value in enum.findall('enum'):
                    # XXX: Skip aliases for now
                    if 'alias' in value.attrib:
                        continue

                    if 'bitpos' in value.attrib:
                        # enum is a bit position
                        Def.Members.append("\t{0} = (1ULL << {1}),".format(value.attrib['name'], value.attrib['bitpos']))
                    elif 'value' in value.attrib:
                        # enum is a value
                        Def.Members.append("{0} = {1},".format(value.attrib['name'], value.attrib['value']))
                EnumDefs[Name] = Def

        else:
            # Just some constants
            for type in enum:
                # XXX: Skip aliases for now
                if 'alias' in type.attrib:
                    continue

                Type = type.attrib['type']
                Name = type.attrib['name']
                BaseType = "constexpr {0} {1} = {2};".format(Type, Name, type.attrib['value'])
                Def = BaseTypeDef(Type, Name, BaseType)
                BaseTypeDefs[Name] = Def

def print_enums():
    for EnumName, Enum in EnumDefs.items():
        print("{0} {1} {{".format(Enum.Type, Enum.Name))
        for Member in Enum.Members:
            print("\t{0}".format(Member))
        print("};")

        for Alias in Enum.Aliases:
            print("typedef {0} {1};".format(Enum.Name, Alias))

        print("\n")

# Walks the types elemnet in the XML and pulls the types out that we care about
# This includes structs, unions, defines, 'basetypes', bitmasks, function pointers
# handles, and enums (aliases only)
def parse_types(root):
    NumStructs = 0
    for types in root.findall('types'):
        for type in types:
            if 'category' in type.attrib:
                if 'requires' in type.attrib:
                    # If what we require is in the skips then skip this
                    Require = type.attrib['requires']
                    if (Require in SkipGenFunctions or
                            Require in SkipExtensionFunctions):
                        continue

                Category = type.attrib['category']
                if (Category == 'struct' or
                    Category == 'union'):
                    Name = type.attrib['name']
                    Def = StructDef(Category, Name)
                    for member in type.findall('member'):
                        for comment in member.findall('comment'):
                            # Struct member has a comment inline
                            # Delete it
                            member.remove(comment)
                        Member = " ".join(member.itertext())
                        Def.Members.append(StructMember("".join(member.find('type').itertext()), "".join(member.find('name').itertext()), Member))

                    # Add additional requirements
                    if Name in AdditionalStructDefines:
                        Def.Requires.append(AdditionalStructDefines[Name])

                    StructDefs[Name] = Def
                    NumStructs += 1

                elif Category == 'define':
                    BaseType = "".join(type.itertext())
                    DefineDefs.append(BaseType)
                elif Category == 'basetype':
                    # Get the basetype name
                    Type = "basetype" # "".join(type.find('type').itertext())
                    Name = "".join(type.find('name').itertext())
                    BaseType = "".join(type.itertext())
                    Def = BaseTypeDef(Type, Name, BaseType)
                    BaseTypeDefs[Name] = Def
                elif Category == 'bitmask':
                    if 'alias' in type.attrib:
                        #<type category="bitmask" name="VkGeometryFlagsNV" alias="VkGeometryFlagsKHR"/>
                        # typedef VkGeometryFlagsKHR VkGeometryFlagsNV;
                        Name = type.attrib['name']
                        FullName =  "typedef {0} {1};".format(type.attrib['alias'], Name)
                        Def = BaseTypeDef(type.attrib['alias'], type.attrib['name'], FullName)
                        BaseTypeDefs[Name] = Def
                    else:
                        # Get the bitmask name
                        Type = "".join(type.find('type').itertext())
                        Name = "".join(type.find('name').itertext())
                        BaseType = "".join(type.itertext())
                        Def = BaseTypeDef(Type, Name, BaseType)
                        BaseTypeDefs[Name] = Def
                elif Category == 'funcpointer':
                    Name = "".join(type.find('name').itertext())
                    FullName = "".join(type.itertext())
                    Def = BaseTypeDef("$funcpointer", Name, FullName)
                    if 'requires' in type.attrib:
                        Def.Requires.append(type.attrib['requires'])
                    BaseTypeDefs[Name] = Def
                elif Category == 'handle':
                    if 'alias' in type.attrib:
                        Name = type.attrib['name']
                        FullName =  "typedef {0} {1};".format(type.attrib['alias'], Name)
                        Def = BaseTypeDef(type.attrib['alias'], Name, FullName)
                        BaseTypeDefs[Name] = Def
                    else:
                        Type = "".join(type.find('type').itertext())
                        Name = "".join(type.find('name').itertext())
                        BaseType = "".join(type.itertext())
                        Def = BaseTypeDef(Type, Name, BaseType)
                        BaseTypeDefs[Name] = Def
                elif Category == 'enum':
                    # Only handle aliases here
                    if 'alias' in type.attrib:
                        Def = EnumDefs[type.attrib['alias']]
                        Def.Aliases.append(type.attrib['name'])

def print_base_type(BaseType):
    if BaseType.Emitted:
        return BaseType

    # Check to see if requirements are already emitted
    for Require in BaseType.Requires:
        if Require in StructDefs:
            # Check if requirement relies on struct
            # if it does and the struct isn't emitted yet then skip it
            if not StructDefs[Require].Emitted:
                return BaseType

    print(BaseType.Str)
    BaseType.Emitted = True
    return BaseType

def print_remaining_base_types():
    for BaseTypeName, BaseType in BaseTypeDefs.items():
        BaseTypeDefs[BaseTypeName] = print_base_type(BaseType)

def print_struct(StructName):
    # If not a struct define type skip it
    if not StructName in StructDefs:
        return

    Struct = StructDefs[StructName]
    # Skip if already emitted
    if Struct.Emitted:
        return

    # Check if our requirements are already emitted
    for Require in Struct.Requires:
        if Require in BaseTypeDefs:
            if not BaseTypeDefs[Require].Emitted:
                return

    Struct.Emitted = True

    # Walk children and emit if necessary
    for member in Struct.Members:
        print_struct(member.Type)

    Annotate = ""
    Annotate = "__attribute__((annotate(\"fex-match\")))"
    print("{0} {1} {2} {{".format(Struct.Type, Annotate, Struct.Name))
    for member in Struct.Members:
        print("\t{0};".format(member.Str))
    print("};")

    # The requirements for base types might have changed
    # print any new ones still remaining
    print_remaining_base_types()

def print_types():
    # Defines first
    for Define in DefineDefs:
        print(Define)

    # Base types second
    print_remaining_base_types()

    # Structs third
    for i in range(0, 2):
        for StructName, Struct in StructDefs.items():
            # First walk the struct members and ensure any dependency is already emitted
            # This ensures correct struct dependency output since the xml doesn't order correctly
            for member in Struct.Members:
                print_struct(member.Type)

            # Now print this struct
            print_struct(Struct.Name)

# Walks the commands element in the XML and pulls out all functions
# This will be used to generate the thunks that we need to hit
def parse_commands(root):
    for command in root.findall('commands'):
        for func in command:
            func_string = ""

            # Skip the alias, will be handled afterwards
            if ('alias' in func.attrib):
                continue

            proto = func.find('proto')
            ReturnType = proto.find('type').text
            Name = proto.find('name').text

            # Skip any function marked as skippable by name
            if (Name in SkipGenFunctions):
                continue

            func_string += ReturnType + " " + Name

            FuncArgs = ""
            params = func.findall('param')
            for i in range(0, len(params)):
                param = params[i]
                # we need to strip out the argument name here
                # Iter all the parameter's text and add it to a list
                ParamDefs = []
                for Def in param.itertext():
                    ParamDefs.append(Def)
                Params = ""

                # Walk the list of parameter defs and drop the final element
                # We don't want the variable name here
                for j in range(0, len(ParamDefs)):
                    if (j + 1) != len(ParamDefs):
                        Params += ParamDefs[j]

                # If you just want everything with the argument definition
                # Params = "".join(param.itertext())

                # Now append the parameter list we generated
                FuncArgs += Params
                if (i + 1) != len(params):
                    FuncArgs += ", "

            FuncDef = FunctionDef(ReturnType, Name, FuncArgs);
            FunctionDefs[Name] = FuncDef

def parse_aliases(root):
    for command in root.findall('commands'):
        for func in command:
            func_string = ""

            if not ('alias' in func.attrib):
                continue

            AliasFunc = func.attrib['alias']
            FuncName = func.attrib['name']

            if (FuncName in SkipGenFunctions):
                continue

            if (AliasFunc in SkipGenFunctions):
                continue

            if (AliasFunc in FunctionDefs):
                OriginalAlias = FunctionDefs[AliasFunc]

                FuncAlias = FunctionDef(OriginalAlias.Ret, FuncName, OriginalAlias.Params)
                FuncAlias.Extensions = OriginalAlias.Extensions
                FunctionDefs[FuncName] = FuncAlias
            else:
                print("Couldn't find alias {0} for {1}".format(AliasFunc, FuncName))
                sys.exit(-1)

def remove_commands(commands):
    for command in commands:
            CommandName = command.attrib['name']
            if CommandName in FunctionDefs:
                del FunctionDefs[CommandName]

def remove_types(types):
    for type in types:
            TypeName = type.attrib['name']
            if TypeName in StructDefs:
                del StructDefs[TypeName]

def remove_extensions(root):
    for extensions in root.findall('extensions'):
        for extension in extensions:
            # First remove anything from the base extension
            if extension.attrib['name'] in SkipExtensionFunctions:
                remove_commands(extension.iter('command'))
                remove_types(extension.iter('type'))

            # Now remove any sub requirements
            for require in extension.findall('require'):
                if 'extension' in require.attrib:
                    if require.attrib['extension'] in SkipExtensionFunctions:
                        remove_commands(require.iter('command'))
                        remove_types(require.iter('type'))

def export_vulkan():
    for FuncName, FuncDef in FunctionDefs.items():
        fn("{0} {1}({2})".format(FuncDef.Ret, FuncDef.Name, FuncDef.Params))

def export_instance_vulkan():
    for FuncName, FuncDef in FunctionDefs.items():
        print("// T({0}),".format(FuncDef.Name))

def parse_vulkan(XML, PrintHeader):
    xml = LoadVulkanXML(XML)
    root = xml.getroot()
    parse_enums(root)
    parse_types(root)
    parse_commands(root)
    # Need to remove extensions before aliases since we don't track extension types through it
    remove_extensions(root)
    parse_aliases(root)

    if PrintHeader:
        # Print some defines that are necessary
        print("#include <cstddef>")
        print("#include <cstdint>")
        print("#include <xcb/xcb.h>")
        print("#include <X11/Xlib.h>")
        print("#include <X11/extensions/Xrandr.h>")
        print("#define VKAPI_ATTR")
        print("#define VKAPI_CALL")
        print("#define VKAPI_PTR")
        print("#define FEX_ANNOTATE(annotation_str) __attribute__((annotate(annotation_str)))")

        print_enums()
        print_types()
        export_instance_vulkan()

if len(sys.argv) < 2:
    print("Usage: {0} libvulkan_{device} vk.xml [--print-header] [--print-thunks]".format(sys.argv[0]))
    sys.exit(0)

LibName = sys.argv[1]
VulkanXML = sys.argv[2]
PrintHeader = False
PrintThunks = False
NumToErase = 2
for i in range(3, len(sys.argv)):
    if sys.argv[i] == '--print-header':
        PrintHeader = True
        NumToErase += 1
    elif sys.argv[i] == '--print-thunks':
        PrintThunks = True
        NumToErase += 1

# Just to fall in line with what the other thunk handlers expect
for i in range(0, NumToErase):
    sys.argv.pop(1)

parse_vulkan(VulkanXML, PrintHeader)

if PrintThunks:
    lib(LibName)

    # Functions here
    export_vulkan()

    fn("PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice,const char*)")
    no_pack()
    no_unpack()

    fn("PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance,const char*)")
    no_pack()
    no_unpack()

    fn("VkResult vkCreateShaderModule(VkDevice,const VkShaderModuleCreateInfo* ,const VkAllocationCallbacks* ,VkShaderModule* )")
    no_unpack()

    fn("void vkCmdSetBlendConstants(VkCommandBuffer, const float*)")
    no_pack()
    no_unpack()

    fn("VkResult vkCreateInstance(const VkInstanceCreateInfo*,const VkAllocationCallbacks*,VkInstance*)")
    no_unpack()

    fn("VkResult vkCreateDevice(VkPhysicalDevice,const VkDeviceCreateInfo*,const VkAllocationCallbacks*,VkDevice*)")
    no_unpack()

    fn("VkResult vkAllocateMemory(VkDevice,const VkMemoryAllocateInfo*,const VkAllocationCallbacks*,VkDeviceMemory*)")
    no_unpack()

    fn("void vkFreeMemory(VkDevice,VkDeviceMemory,const VkAllocationCallbacks*)")
    no_unpack()

    fn("VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice,uint32_t,VkSurfaceKHR,VkBool32*)")
    no_unpack()

    Generate()
