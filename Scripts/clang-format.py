# Imported from LLVM
# It's basically a clang-format wrapper with .clang-format-ignore support.
# Can be removed once we adopt Clang19, which supports this out of the box.
import subprocess
import sys
import os
import re
import fnmatch

# Wrapper globals
project_root = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..")
ignore_file_path = os.path.join(project_root, ".clang-format-ignore")
clang_format_command = os.getenv("CLANG_FORMAT") or "clang-format"


def glob_to_regex(pattern):
    # Normalize directory separators
    pattern = pattern.replace("\\", "/")
    return fnmatch.translate(pattern)


def load_ignore_patterns(ignore_file_path):
    with open(ignore_file_path, "r") as file:
        lines = file.readlines()

    patterns = []
    for line in lines:
        line = line.strip()
        if line and not line.startswith("#"):  # Ignore empty lines and comments
            pattern = glob_to_regex(line)
            patterns.append(re.compile(pattern))
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


def find_valid_file_paths(args):
    return [arg for arg in args if os.path.isfile(arg)]


def main():
    ignore_patterns = load_ignore_patterns(ignore_file_path)
    valid_paths = find_valid_file_paths(sys.argv[1:])

    if len(valid_paths) != 1:
        print("Error: Expected exactly one valid file path as argument.")
        sys.exit(1)

    file_path = valid_paths[0]
    if should_ignore(file_path, ignore_patterns):
        print(f"Ignoring {file_path} based on ignore patterns.")
        return

    subprocess.run([clang_format_command] + sys.argv[1:], check=True)


if __name__ == "__main__":
    main()
