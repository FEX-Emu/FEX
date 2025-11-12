#!/usr/bin/python3
import sys
import subprocess
from os import path
from shutil import which

# Args: <Known Failures file> <Known Failures Type File> <DisabledTestsFile> <DisabledTestsTypeFile> <DisabledTestsRunnerFile> <TestName> <FullTestName> <Test Harness Executable> <Args>...

if (len(sys.argv) < 8):
    sys.exit()

known_failures = {}
disabled_tests = {}
known_failures_file = sys.argv[1]
known_failures_type_file = sys.argv[2]
disabled_tests_file = sys.argv[3]
disabled_tests_type_file = sys.argv[4]
disabled_tests_runner_file = sys.argv[5]

current_test = sys.argv[6]
full_test_name = sys.argv[7]
runner = sys.argv[8]
args_start_index = 9

# Open the known failures file and add it to a dictionary
with open(known_failures_file) as kff:
    for line in kff:
        known_failures[line.strip()] = 1

if path.exists(known_failures_type_file):
    with open(known_failures_type_file) as dtf:
        for line in dtf:
            known_failures[line.strip()] = 1

with open(disabled_tests_file) as dtf:
    for line in dtf:
        disabled_tests[line.strip()] = 1

if path.exists(disabled_tests_type_file):
    with open(disabled_tests_type_file) as dtf:
        for line in dtf:
            disabled_tests[line.strip()] = 1

if path.exists(disabled_tests_runner_file):
    with open(disabled_tests_runner_file) as dtf:
        for line in dtf:
            disabled_tests[line.strip()] = 1

RunnerArgs = ["catchsegv", runner]

if which("catchsegv") is None:
    RunnerArgs.pop(0)
# Add the rest of the arguments
for i in range(len(sys.argv) - args_start_index):
    RunnerArgs.append(sys.argv[args_start_index + i])

if (disabled_tests.get(current_test)):
    # This error code tells ctest that the test was skipped
    sys.exit(125)

# Run the test and wait for it to end to get the result
Process = subprocess.Popen(RunnerArgs)
Process.wait()
ResultCode = Process.returncode

# Check for known failures - try full test name first, then partial test name
is_known_failure = known_failures.get(full_test_name) or known_failures.get(current_test)

if (is_known_failure):
    # If the test is on the known failures list
    if (ResultCode):
        # If we errored but are on the known failures list then "pass" the test
        sys.exit(0)
    else:
        # If we didn't error but are in the known failure list then we need to fail the test
        sys.exit(1)
else:
    # Just return the result code if we don't have this test as a known failure
    sys.exit(ResultCode)
