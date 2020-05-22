#!/usr/bin/python3
import sys
import subprocess

# Args: <Known Failures file> <ExpectedOutputsFile> <TestName> <FexExecutable> <FexArgs>...

# fexargs should also include the test executable

if (len(sys.argv) < 6):
    sys.exit()

known_failures_file = sys.argv[1]
expected_output_file = sys.argv[2]
test_name = sys.argv[3]

known_failures = { }
expected_output = { }

# Open the known failures file and add it to a dictionary
with open(known_failures_file) as kff:
    for line in kff:
        known_failures[line.strip()] = 1

# Open expected outputs and add it to dictionary
with open(expected_output_file) as eof:
    for line in eof:
        parts = line.strip().split(" ")
        expected_output[parts[0]] = int(parts[1])

# run with timeout to avoid locking up
RunnerArgs = ["timeout", "45s"]

# Add the rest of the arguments
for i in range(len(sys.argv) - 4):
    RunnerArgs.append(sys.argv[4 + i])

#print(RunnerArgs)

# Run the test and wait for it to end to get the result
Process = subprocess.Popen(RunnerArgs)
Process.wait()
ResultCode = Process.returncode

if (expected_output[test_name] != ResultCode):
    if (known_failures.get(test_name)):
        # failed and expected to fail -- pass the test
        sys.exit(0)
    else:
        # failed and unexpected to fail -- fail the test
        sys.exit(1)
else:
    if (known_failures.get(test_name)):
        # passed and expected to fail -- fail the test
        sys.exit(1)
    else:
        # passed and expected to pass -- pass the test
        sys.exit(0)

