#!/usr/bin/python3
import sys
import subprocess

# Args: <Known Failures file> <DisabledTestsFile> <TestName> <Test Harness Executable> <Args>...

if (len(sys.argv) < 5):
    sys.exit()

known_failures = {}
disabled_tests = {}
known_failures_file = sys.argv[1]
disabled_tests_file = sys.argv[2]

current_test = sys.argv[3]

# Open the known failures file and add it to a dictionary
with open(known_failures_file) as kff:
    for line in kff:
        known_failures[line.strip()] = 1

with open(disabled_tests_file) as dtf:
    for line in dtf:
        disabled_tests[line.strip()] = 1

RunnerArgs = ["catchsegv", sys.argv[4]]
# Add the rest of the arguments
for i in range(len(sys.argv) - 5):
    RunnerArgs.append(sys.argv[5 + i])

if (disabled_tests.get(current_test)):
    print("Skipping", current_test)
    sys.exit(0)

# Run the test and wait for it to end to get the result
Process = subprocess.Popen(RunnerArgs)
Process.wait()
ResultCode = Process.returncode

if (known_failures.get(current_test)):
    # If the test is on the known failures list
    if (ResultCode):
        # If we errored but are on the known failures list then "pass" the test
        sys.exit(0)
    else:
        # If we didn't error but are in the known failure list then we need to fail the test
        sys.exit(1)
else:
    # Just return the result code if we don't have this test as a known failure
    sys.exit(ResultCode);

