#!/usr/bin/python3
import json
import os
import sys
import subprocess

# Check if FEX indicates support for AVX
def DoesFEXSupportAVX(mode):
    fex_interpreter_path = os.path.dirname(sys.argv[7]) + "/FEXInterpreter"

    args = list()
    args.append(fex_interpreter_path)
    if (mode == "guest"):
        ROOTFS_ENV = os.getenv("ROOTFS")
        if ROOTFS_ENV != None:
            args.append("-R")
            args.append(ROOTFS_ENV)
    args.append('/bin/cat')
    args.append('/proc/cpuinfo')

    process = subprocess.run(args, capture_output=True, text=True)
    output = process.stdout

    for line in output:
        if 'flags' in line:
            flags = line.split(':')[1].strip().split(' ')
            return 'avx' in flags and 'avx2' in flags
    return False

# Check if the test itself requires AVX
def TestRequiresAVXSupport():
    exe_path = sys.argv[len(sys.argv) - 1]
    json_path = os.path.dirname(os.path.dirname(exe_path)) + '/requirements/' + os.path.basename(exe_path) + '.json'

    try:
        with open(json_path) as json_file:
            try:
                json_data = json.load(json_file)
                if not isinstance(json_data, dict):
                    raise TypeError('JSON data must be a dict')

                if "AVX" in json_data["HostFeatures"]:
                    return True
            except ValueError as ve:
                print(f'JSON error: {ve}')
                pass
    except IOError:
        # If we get here, then we don't have a corresponding JSON
        # file for the associated test, and can assume there's no
        # feature requirements for the test.
        pass
    return False

def LoadTestsFile(File):
    Dict = {}
    if not os.path.exists(File):
        return Dict

    with open(File) as dtf:
        for line in dtf:
            test = line.split("#")[0].strip() # remove comments and empty spaces
            if len(test) > 0:
                Dict[test] = 1

    return Dict

def LoadTestsFileResults(File):
    Dict = {}
    if not os.path.exists(File):
        return Dict

    with open(File) as dtf:
        for line in dtf:
            test = line.split("#")[0].strip() # remove comments and empty spaces
            if len(test) > 0:
                parts = line.split(" ")
                Dict[parts[0]] = int(parts[1])

    return Dict


# Args: <Known Failures file> <ExpectedOutputsFile> <DisabledTestsFile> <FlakeTestsFile> <TestName> <Mode> <FexExecutable> <FexArgs>...

# fexargs should also include the test executable

if (len(sys.argv) < 7):
    sys.exit()

known_failures_file = sys.argv[1]
expected_output_file = sys.argv[2]
disabled_tests_file = sys.argv[3]
flake_tests_file = sys.argv[4]
test_name = sys.argv[5]
mode = sys.argv[6]
fexecutable = sys.argv[7]
StartingFEXArgsOffset = 8

# If the test requires AVX and FEX doesn't support it, just pass the test and move on
if TestRequiresAVXSupport() and not DoesFEXSupportAVX(mode):
    sys.exit(0)

# Open test expected information files and load in to dictionaries.
known_failures = LoadTestsFile(known_failures_file)
expected_output = LoadTestsFileResults(expected_output_file)
disabled_tests = LoadTestsFile(disabled_tests_file)
flake_tests = LoadTestsFile(flake_tests_file)

# run with timeout to avoid locking up
RunnerArgs = []

RunnerArgs.append(fexecutable)

if (mode == "guest"):
    ROOTFS_ENV = os.getenv("ROOTFS")
    if ROOTFS_ENV != None:
        RunnerArgs.append("-R")
        RunnerArgs.append(ROOTFS_ENV)

# Add the rest of the arguments
for i in range(len(sys.argv) - StartingFEXArgsOffset):
    RunnerArgs.append(sys.argv[StartingFEXArgsOffset + i])

# print(RunnerArgs)

ResultCode = 0

# Handle flakes
TryCount = 1
if (flake_tests.get(test_name)):
    TryCount = 5

if (disabled_tests.get(test_name)):
    print(f"Test {test_name} is disabled")
    sys.exit(0)

# expect zero by default
if (not test_name in expected_output):
    expected_output[test_name] = 0

if ResultCode == 0:
    for Try in range(TryCount):
        # Run the test and wait for it to end to get the result
        print(RunnerArgs)
        Process = subprocess.Popen(RunnerArgs)
        Process.wait()
        ResultCode = Process.returncode

        # Break if the expected output is the result code
        if (expected_output[test_name] == ResultCode):
            break

if (expected_output[test_name] != ResultCode):
    if (test_name in expected_output):
        print("test failed, expected is", expected_output[test_name], "but got", ResultCode)
    else:
        print("Test doesn't have expected output,", test_name)

    if (known_failures.get(test_name)):
        print("Passing because it was expected to fail")
        # failed and expected to fail -- pass the test
        sys.exit(0)
    else:
        # failed and unexpected to fail -- fail the test
        sys.exit(1)
else:
    print("test passed with", ResultCode)
    if (known_failures.get(test_name)):
        print("Failing because it was expected to fail")
        # passed and expected to fail -- fail the test
        sys.exit(1)
    else:
        # passed and expected to pass -- pass the test
        sys.exit(0)
