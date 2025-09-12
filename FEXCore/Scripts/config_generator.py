import datetime
import json
import sys

def print_header():
    header = '''#ifndef OPT_BASE
#define OPT_BASE(type, group, enum, json, default)
#endif
#ifndef OPT_BOOL
#define OPT_BOOL(group, enum, json, default) OPT_BASE(bool, group, enum, json, default)
#endif
#ifndef OPT_UINT8
#define OPT_UINT8(group, enum, json, default) OPT_BASE(uint8_t, group, enum, json, default)
#endif
#ifndef OPT_INT32
#define OPT_INT32(group, enum, json, default) OPT_BASE(int32_t, group, enum, json, default)
#endif
#ifndef OPT_UINT32
#define OPT_UINT32(group, enum, json, default) OPT_BASE(uint32_t, group, enum, json, default)
#endif
#ifndef OPT_UINT64
#define OPT_UINT64(group, enum, json, default) OPT_BASE(uint64_t, group, enum, json, default)
#endif
#ifndef OPT_STR
#define OPT_STR(group, enum, json, default) OPT_BASE(fextl::string, group, enum, json, default)
#endif
#ifndef OPT_STRARRAY
#define OPT_STRARRAY(group, enum, json, default) OPT_BASE(fextl::string, group, enum, json, default)
#endif
#ifndef OPT_STRENUM
#define OPT_STRENUM(group, enum, json, default) OPT_BASE(uint64_t, group, enum, json, default)
#endif
'''
    output_file.write(header)

def print_tail():
    tail = '''#undef OPT_BASE
#undef OPT_BOOL
#undef OPT_UINT8
#undef OPT_INT32
#undef OPT_UINT32
#undef OPT_UINT64
#undef OPT_STR
#undef OPT_STRARRAY
#undef OPT_STRENUM
'''
    output_file.write(tail)

def print_config(type, group_name, json_name, default_value):
    output_file.write("OPT_{0} ({1}, {2}, {3}, {4})\n".format(type.upper(), group_name.upper(), json_name.upper(), json_name, default_value))

def print_options(options):
    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            default = op_vals["Default"]
            if (op_vals["Type"] == "str" or op_vals["Type"] == "strarray"):
                # Wrap the string argument in quotes
                default = "\"" + default + "\""

            print_config(
                op_vals["Type"],
                op_group,
                op_key,
                default)

        output_file.write("\n")

def print_unnamed_options(options):
    output_file.write("// Unnamed configuration options\n")
    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            default = op_vals["Default"]
            if (op_vals["Type"] == "str" or op_vals["Type"] == "strarray"):
                # Wrap the string argument in quotes
                default = "\"" + default + "\""

            print_config(
                op_vals["Type"],
                op_group,
                op_key.upper(), # KEY is the enum here, there is no json configuration for these
                default)

        output_file.write("\n")

def print_man_option(short, long, desc, default):
    if (short != None):
        output_man.write(".It Fl {0} , ".format(short))
    else:
        output_man.write(".It ")

    output_man.write("Fl Fl {0}=".format(long))

    output_man.write("\n");

    # Print description
    for line in desc:
        output_man.write(".Pp\n")
        output_man.write("{0}\n".format(line))

    output_man.write(".Pp\n")
    output_man.write("\\fBdefault:\\fR {0}\n".format(default))
    output_man.write(".Pp\n\n")

def print_man_env_option(name, desc, default, no_json_key):
    output_man.write("\\fBFEX_{0}\\fR\n".format(name.upper()))

    # Print description
    for line in desc:
        output_man.write(".Pp\n")
        output_man.write("{0}\n".format(line))

    if (not no_json_key):
        output_man.write(".Pp\n")
        output_man.write("\\fBJSON key:\\fR '{0}'\n".format(name))
        output_man.write(".Pp\n\n")

    output_man.write(".Pp\n")
    output_man.write("\\fBdefault:\\fR {0}\n".format(default))
    output_man.write(".Pp\n\n")

def print_man_environment(options):
    output_man.write(".Sh ENVIRONMENT\n")
    output_man.write(".Bl -tag -width -indent\n")
    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            default = op_vals["Default"]
            value_type = op_vals["Type"]

            # Textual default rather than enum based
            if ("TextDefault" in op_vals):
                default = op_vals["TextDefault"]

            if (value_type == "str" or value_type == "strarray" or value_type == "strenum"):
                # Wrap the string argument in quotes
                default = "'" + default + "'"
            print_man_env_option(
                op_key,
                op_vals["Desc"],
                default,
                False
            )

            if (value_type == "strenum"):
                Enums = op_vals["Enums"]
                output_man.write("\\fBAvailable Options:\\fR\n")
                output_man.write(", ".join(f"{enum_op_val}" for [_, enum_op_val] in Enums.items()))
                output_man.write("\n.sp\n")

    print_man_environment_tail()
    output_man.write(".El\n")

def print_man_environment_tail():

    # Additional environment variables that live outside of the normal loop
    print_man_env_option(
    "APP_CONFIG_LOCATION",
    [
    "Allows the user to override where FEX looks for configuration files",
    "By default FEX will look in {$HOME, $XDG_CONFIG_HOME}/.fex-emu/",
    "This will override the full path",
    "If FEX_PORTABLE is declared then relative paths are also supported",
    "For FEX: Relative to the FEX binary",
    "For WINE: Relative to %LOCALAPPDATA%"
    ],
    "''", True)

    print_man_env_option(
    "APP_CONFIG",
    [
    "Allows the user to override where FEX looks for only the application config file",
    "By default FEX will look in {$HOME, $XDG_CONFIG_HOME}/.fex-emu/Config.json",
    "This will override this file location",
    "One must be careful with this option as it will override any applications that load with execve as well"
    "If you need to support applications that execve then use FEX_APP_CONFIG_LOCATION instead"
    "If FEX_PORTABLE is declared then relative paths are also supported",
    "For FEX: Relative to the FEX binary",
    "For WINE: Relative to %LOCALAPPDATA%"
    ],
    "''", True)

    print_man_env_option(
    "APP_DATA_LOCATION",
    [
    "Allows the user to override where FEX looks for data files",
    "By default FEX will look in {$HOME, $XDG_DATA_HOME}/.fex-emu/",
    "This will override the full path",
    "This is the folder where FEX stores generated files like IR cache"
    ],
    "''", True)

    print_man_env_option(
    "PORTABLE",
    [
    "Allows FEX to run without installation. Global locations for configuration and binfmt_misc are ignored.",
    "For FEX on Linux:",
    "These files are instead read from <FEXPath>/fex-emu/ by default.",
    "For Arm64ec/Wow64 WINE builds:",
    "These files are instead read from $LOCALAPPDATA/fex-emu/ by default.",
    "For further customization, see FEX_APP_CONFIG_LOCATION and FEX_APP_DATA_LOCATION."
    ],
    "''", True)

def print_man_header():
    header ='''.Dd {0}
.Dt FEX
.Os Linux
.Sh NAME
.Nm FEX
.Nm FEXBash
.Nd Fast x86-64 and x86 emulation.
.Sh SYNOPSIS
.Nm
.Ar <args> ...
.Pp
.Nm FEXBash
.Ar <args> ...
.Sh DESCRIPTION
FEX allows you to run x86 and x86-64 binaries on an AArch64 host, similar to qemu-user and box86.
It has native support for a rootfs overlay, so you don't need to chroot, as well as some thunklibs so it can forward things like GL to the host.
FEX presents a Linux 5.0 interface to the guest, and supports both AArch64 and x86-64 as hosts.
FEX is very much work in progress, so expect things to change.
'''
    output_man.write(header.format(datetime.datetime.now().strftime("%d-%m-%Y")))

def print_man_tail():
    tail ='''.Sh FILES
.Bl -tag -width "$prefix/share/fex-emu/GuestThunks" -compact
.It Pa $XDG_HOME_DIR/.fex-emu
Default FEX user configuration directory
.It Pa $prefix/share/fex-emu/AppConfig
System level application configuration files
.It Pa $prefix/share/fex-emu/GuestThunks
guest-side thunk data libraries
.It Pa $prefix/lib/fex-emu/HostThunks
host-side thunks for guest communication
.El
'''
    output_man.write(tail)

def print_config_option(type, group_name, json_name, default_value, short, choices, desc):
    if (type == "bool"):
        # Bool gets some special handling to add an inverted case
        output_argloader.write("{0}Group".format(group_name))

        options = ""
        AddedArg = False
        if (short != None):
            AddedArg = True
            options += "\"-{0}\"".format(short)

        if (AddedArg):
            options += ", "
        options += "\"--{0}\"".format(json_name.lower())

        output_argloader.write(".add_option({0})".format(options))

        output_argloader.write("\n")

        output_argloader.write("\t.action(\"store_true\")\n")

        output_argloader.write("\t.dest(\"{0}\")\n".format(json_name));

        # help
        output_argloader.write("\t.help(\n")
        desc_line_ender = ""
        if (len(desc) > 1):
            desc_line_ender = "\\n"

        for line in desc:
            output_argloader.write("\t\t\"{0}{1}\"\n".format(line, desc_line_ender))
        output_argloader.write("\t)\n")

        output_argloader.write("\t.set_default({0});\n\n".format(default_value));

        output_argloader.write("{0}Group".format(group_name))
        output_argloader.write(".add_option(\"--no-{0}\")\n".format(json_name.lower()))

        # Inverted case sets the bool to false
        output_argloader.write("\t.action(\"store_false\")\n")

        output_argloader.write("\t.dest(\"{0}\");\n".format(json_name));
    else:
        output_argloader.write("{0}Group".format(group_name))
        options = ""
        AddedArg = False
        if (short != None):
            AddedArg = True
            options += "\"-{0}\"".format(short)

        if (AddedArg):
            options += ", "
        options += "\"--{0}\"".format(json_name.lower())

        output_argloader.write(".add_option({0})".format(options))

        output_argloader.write("\n")

        output_argloader.write("\t.dest(\"{0}\")\n".format(json_name));

        if (choices != None):
            output_argloader.write("\t.choices({\n")
            for choice in choices:
                output_argloader.write("\t\t\"{0}\",\n".format(choice))
            output_argloader.write("\t})\n")


        # help
        output_argloader.write("\t.help(\n")
        desc_line_ender = ""
        if (len(desc) > 1):
            desc_line_ender = "\\n"

        for line in desc:
            output_argloader.write("\t\t\"{0}{1}\"\n".format(line, desc_line_ender))
        output_argloader.write("\t)\n")

        output_argloader.write("\t.set_default({0});\n".format(default_value));

    output_argloader.write("\n");

def print_parse_envloader_options(options):
    output_argloader.write("#ifdef ENVLOADER\n")
    output_argloader.write("#undef ENVLOADER\n")
    output_argloader.write("if (false) {}\n")

    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            value_type = op_vals["Type"]
            if (value_type == "strenum"):
                output_argloader.write("else if (Key == \"FEX_{0}\") {{\n".format(op_key.upper()))
                output_argloader.write("\tValue = FEXCore::Config::EnumParser<FEXCore::Config::{}ConfigPair>(FEXCore::Config::{}_EnumPairs, Value_View);\n".format(op_key, op_key))
                output_argloader.write("}\n")

            if ("ArgumentHandler" in op_vals):
                conversion_func = "FEXCore::Config::Handler::{0}".format(op_vals["ArgumentHandler"])
                output_argloader.write("else if (Key == \"FEX_{0}\") {{\n".format(op_key.upper()))
                output_argloader.write("\tValue = {0}(Value_View);\n".format(conversion_func))
                output_argloader.write("}\n")
    output_argloader.write("#endif\n")

def print_parse_jsonloader_options(options):
    output_argloader.write("#ifdef JSONLOADER\n")
    output_argloader.write("#undef JSONLOADER\n")
    output_argloader.write("if (false) {}\n")
    op_key = None
    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            value_type = op_vals["Type"]
            if (value_type == "strenum"):
                output_argloader.write("else if (KeyName == \"{0}\") {{\n".format(op_key))
                output_argloader.write("\tSet(KeyOption, FEXCore::Config::EnumParser<FEXCore::Config::{}ConfigPair>(FEXCore::Config::{}_EnumPairs, Value_View));\n".format(op_key, op_key))
                output_argloader.write("}\n")
            elif (value_type == "strarray"):
                output_argloader.write("else if (KeyName == \"{0}\") {{\n".format(op_key))
                output_argloader.write("\tAppendStrArrayValue(KeyOption, ConfigString);\n")
                output_argloader.write("}\n")
    assert op_key is not None, "No options found in JSONLOADER"
    output_argloader.write("else {\n")
    output_argloader.write("\tSet(KeyOption, ConfigString);\n")
    output_argloader.write("}\n")

    output_argloader.write("#endif\n")

def print_parse_enum_options(options):
    output_argloader.write("#ifdef ENUMDEFINES\n")
    output_argloader.write("#undef ENUMDEFINES\n")
    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            if (op_vals["Type"] == "strenum"):
                output_argloader.write("enum class {} : uint64_t {{\n".format(op_key))
                Enums = op_vals["Enums"]
                i = 0
                # Always have an OFF.
                output_argloader.write("\tOFF = 0,\n")
                for enum_op_key, enum_op_vals in Enums.items():
                    output_argloader.write("\t{} = 1ULL << {},\n".format(enum_op_key.upper(), i))
                    i += 1

                output_argloader.write("};\n")
                output_argloader.write("FEX_DEF_NUM_OPS({})\n".format(op_key))


    for op_group, group_vals in options.items():
        for op_key, op_vals in group_vals.items():
            if (op_vals["Type"] == "strenum"):
                Enums = op_vals["Enums"]

                output_argloader.write("using {}ConfigPair = std::pair<std::string_view, FEXCore::Config::{}>;\n".format(op_key, op_key))
                output_argloader.write("constexpr static std::array<{}ConfigPair, {}> {}_EnumPairs = {{{{\n".format(op_key, len(Enums) + 1, op_key))
                i = 0
                # Always have an OFF.
                output_argloader.write("\t{{ \"off\", FEXCore::Config::{}::OFF }},\n".format(op_key))
                for enum_op_key, enum_op_vals in Enums.items():
                    output_argloader.write("\t{{ \"{}\", FEXCore::Config::{}::{} }},\n".format(enum_op_vals, op_key, enum_op_key.upper()))
                    i += 1

                output_argloader.write("}};\n")

    output_argloader.write("#endif\n")

if (len(sys.argv) < 5):
    sys.exit()

output_filename = sys.argv[2]
output_man_page = sys.argv[3]
output_argumentloader_filename = sys.argv[4]

json_file = open(sys.argv[1], "r")
json_text = json_file.read()
json_file.close()

json_object = json.loads(json_text)

options = json_object["Options"]
unnamed_options = json_object["UnnamedOptions"]

# Generate config include file
output_file = open(output_filename, "w")
print_header()
print_options(options)
print_unnamed_options(unnamed_options)
print_tail()
output_file.close()

# Generate man file
output_man = open(output_man_page, "w")
print_man_header()
print_man_environment(options)
print_man_tail()

output_man.close()

# Generate argument loader code
output_argloader = open(output_argumentloader_filename, "w")

# Generate environment loader code
print_parse_envloader_options(options);

# Generate json loader code
print_parse_jsonloader_options(options);

# Generate enum variable options
print_parse_enum_options(options);

output_argloader.close()
