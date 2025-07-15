#!/usr/bin/env python3
# Imported from LLVM
# It's basically a clang-format wrapper with .clang-format-ignore support.
# Can be removed once we adopt Clang19, which supports this out of the box.
#
# This is called by git-clang-format and it works in two modes.
# It has a path in the command line to format, and it will output the result post format to stdout.
# Or it has -assume-filename=<path> in the command line, and the file is passed via stdin.
# Post format output will be given in stdout.

import subprocess
import sys
import os
import re
import fnmatch

# Wrapper globals
project_root = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..")
ignore_file_path = os.path.join(project_root, ".clang-format-ignore")
clang_format_command = os.getenv("CLANG_FORMAT") or "clang-format-19"


def glob_to_regex(pattern):
    # Normalize directory separators
    pattern = pattern.replace("\\", "/")
    return fnmatch.translate(pattern)


def load_ignore_patterns(ignore_file_path):
    # Check if the file exists
    if not os.path.exists(ignore_file_path):
        raise FileNotFoundError(f"No such file: '{ignore_file_path}'")

    with open(ignore_file_path, "r") as file:
        lines = file.readlines()

    patterns = []
    for line in lines:
        line = line.strip()
        if line and not line.startswith("#"):  # Ignore empty lines and comments
            pattern = glob_to_regex(line)
            patterns.append(re.compile(pattern))

    # Print the number of patterns found
    print(f"Number of patterns found: {len(patterns)}", file=sys.stderr)

    return patterns


def normalize_path(file_path):
    absolute_path = os.path.abspath(file_path)
    normalized_path = absolute_path.replace("\\", "/")
    return normalized_path


def should_ignore(file_path, ignore_patterns):
    normalized_path = normalize_path(file_path)
    relative_path = os.path.relpath(normalized_path, start=project_root).replace(
        "\\", "/"
    )
    for pattern in ignore_patterns:
        if pattern.match(relative_path):
            return True
    return False


def main():
    ignore_patterns = load_ignore_patterns(ignore_file_path)
    assume_filename = None
    args = sys.argv[1:]

    # Extract and handle `-assume-filename=<filename>`
    args_filtered = []
    for arg in args:
        if arg.startswith("-assume-filename="):
            _, assume_filename = arg.split("=", 1)
        else:
            args_filtered.append(arg)

    args = args_filtered

    if assume_filename is not None:
        if should_ignore(assume_filename, ignore_patterns):
            print(
                f"Ignoring {assume_filename} based on ignore patterns.", file=sys.stderr
            )
            sys.stdout.write(sys.stdin.read())
            sys.exit(0)
        input_stream = sys.stdin.read()
        subprocess.run(
            [clang_format_command, "-assume-filename=" + assume_filename] + args,
            input=input_stream.encode(),
            check=True,
        )
    else:
        # Find all valid file paths
        valid_paths = [arg for arg in args if os.path.isfile(arg)]
        if len(valid_paths) != 1:
            print(
                "Error: Exactly one valid file path is required when -assume-filename is not present.",
                file=sys.stderr,
            )
            sys.exit(1)

        file_path = valid_paths[0]
        if should_ignore(file_path, ignore_patterns):
            print(f"Ignoring {file_path} based on ignore patterns.", file=sys.stderr)
            with open(file_path, "r") as file:
                sys.stdout.write(file.read())
            sys.exit(0)

        subprocess.run([clang_format_command] + args, check=True)


if __name__ == "__main__":
    main()
