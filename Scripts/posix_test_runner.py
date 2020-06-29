#!/usr/bin/python3
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

known_failures = { }
expected_output = { }
disabled_tests = { }

# Open the known failures file and add it to a dictionary
with open(known_failures_file) as kff:
    for line in kff:
        known_failures[line.strip()] = 1

# Open expected outputs and add it to dictionary
with open(expected_output_file) as eof:
    for line in eof:
        parts = line.strip().split(" ")
        expected_output[parts[0]] = int(parts[1])

with open(disabled_tests_file) as dtf:
    for line in dtf:
        disabled_tests[line.strip()] = 1

# run with timeout to avoid locking up
RunnerArgs = ["timeout", "300s"]

# Add the rest of the arguments
for i in range(len(sys.argv) - 5):
    RunnerArgs.append(sys.argv[5 + i])

#print(RunnerArgs)

if (disabled_tests.get(test_name)):
    print("Skipping", test_name)
    sys.exit(0)

# Run the test and wait for it to end to get the result
Process = subprocess.Popen(RunnerArgs)
Process.wait()
ResultCode = Process.returncode

if (expected_output[test_name] != ResultCode):
    print("test failed, expected is", expected_output[test_name], "but got", ResultCode)
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

