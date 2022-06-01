#!/usr/bin/python3
import os
import sys
import subprocess

# Args: <Known Failures file> <ExpectedOutputsFile> <DisabledTestsFile> <TestName> <FexExecutable> <FexArgs>...

# fexargs should also include the test executable

if (len(sys.argv) < 6):
    sys.exit()

known_failures_file = sys.argv[1]
expected_output_file = sys.argv[2]
disabled_tests_file = sys.argv[3]
test_name = sys.argv[4]
mode = sys.argv[5]
fexecutable = sys.argv[6]

known_failures = { }
expected_output = { }
disabled_tests = { }

# Open the known failures file and add it to a dictionary
with open(known_failures_file) as kff:
    for line in kff:
        test = line.split("#")[0].strip() # remove comments and empty spaces
        if len(test) > 0:
            known_failures[test] = 1

# Open expected outputs and add it to dictionary
with open(expected_output_file) as eof:
    for line in eof:
        line = test = line.split("#")[0].strip() # remove comments and empty spaces
        if len(line) > 0:
            parts = line.split(" ")
            expected_output[parts[0]] = int(parts[1])

with open(disabled_tests_file) as dtf:
    for line in dtf:
        test = line.split("#")[0].strip() # remove comments and empty spaces
        if len(test) > 0:
            disabled_tests[test] = 1

# run with timeout to avoid locking up
RunnerArgs = []

RunnerArgs.append(fexecutable)

if (mode == "guest"):
    ROOTFS_ENV = os.getenv("ROOTFS")
    if ROOTFS_ENV != None:
        RunnerArgs.append("-R")
        RunnerArgs.append(ROOTFS_ENV)

# Add the rest of the arguments
for i in range(len(sys.argv) - 7):
    RunnerArgs.append(sys.argv[7 + i])

#print(RunnerArgs)

ResultCode = 0

if (disabled_tests.get(test_name)):
    ResultCode = -73
else:
    # Run the test and wait for it to end to get the result
    Process = subprocess.Popen(RunnerArgs)
    Process.wait()
    ResultCode = Process.returncode

# expect zero by default
if (not test_name in expected_output):
    expected_output[test_name] = 0

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

