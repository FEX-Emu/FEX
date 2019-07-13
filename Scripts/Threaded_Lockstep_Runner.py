#!/usr/bin/python3
from enum import Flag
import json
import os
import struct
import sys
import glob
from threading import Thread
import subprocess
import time
import multiprocessing

if sys.version_info[0] < 3:
        raise Exception("Python 3 or a more recent version is required.")

if (len(sys.argv) < 3):
    sys.exit("We need two arguments. Location of LockStepRunner and folder containing the tests")

# Remove our SHM regions if they still exist
SHM_Files = glob.glob("/dev/shm/*_Lockstep")
for file in SHM_Files:
    os.remove(file)

UnitTests = sorted(glob.glob(sys.argv[2] + "*"))
UnitTestsSize = len(UnitTests)
Threads = [None] * UnitTestsSize
Results = [None] * UnitTestsSize
ThreadResults = [[None] * 2] * UnitTestsSize
MaxFileNameStringLen = 0

def Threaded_Runner(Args, ID, Client):
    Log = open("Log_" + str(ID) + "_" + str(Client), "w")
    Log.write("Args: %s\n" % " ".join(Args))
    Log.flush()
    Process = subprocess.Popen(Args, stdout=Log, stderr=Log)
    Process.wait()
    Log.flush()
    ThreadResults[ID][Client] = Process.returncode

def Threaded_Manager(Runner, ID, File):
    ServerArgs = ["catchsegv", Runner, "-c", "vm", "-n", "1", "-I", "R" + str(ID), File]
    ClientArgs = ["catchsegv", Runner, "-c", "vm", "-n", "1", "-I", "R" + str(ID), "-C"]

    ServerThread = Thread(target = Threaded_Runner, args = (ServerArgs, ID, 0))
    ClientThread = Thread(target = Threaded_Runner, args = (ClientArgs, ID, 1))

    ServerThread.start()
    ClientThread.start()

    ClientThread.join()
    ServerThread.join()

    # The server is the one we should listen to for results
    if (ThreadResults[ID][1] != 0 and ThreadResults[ID][0] == 0):
        # If the client died for some reason but server thought we were fine then take client data
        Results[ID] = ThreadResults[ID][1]
    else:
        # Else just take the server data
        Results[ID] = ThreadResults[ID][0]

    DupLen = MaxFileNameStringLen - len(UnitTests[ID])

    if (Results[ID] == 0):
        print("\t'%s'%s - PASSED ID: %d - 0" % (UnitTests[ID], " "*DupLen, ID))
    else:
        print("\t'%s'%s - FAILED ID: %d - %s" % (UnitTests[ID], " "*DupLen, ID, hex(Results[ID])))

RunnerSlot = 0
MaxRunnerSlots = min(32, multiprocessing.cpu_count() / 2)
RunnerSlots = [None] * MaxRunnerSlots
for RunnerID in range(UnitTestsSize):
    File = UnitTests[RunnerID]
    print("'%s' Running Test" % File)
    MaxFileNameStringLen = max(MaxFileNameStringLen, len(File))
    Threads[RunnerID] = Thread(target = Threaded_Manager, args = (sys.argv[1], RunnerID, File))
    Threads[RunnerID].start()
    if (MaxRunnerSlots != 0):
        RunnerSlots[RunnerSlot] = Threads[RunnerID]
        RunnerSlot += 1
        if (RunnerSlot == MaxRunnerSlots):
            for i in range(MaxRunnerSlots):
                RunnerSlots[i].join()
            RunnerSlot = 0

for i in range(UnitTestsSize):
    Threads[i].join()

print("====== PASSED RESULTS ======")
for i in range(UnitTestsSize):
    DupLen = MaxFileNameStringLen - len(UnitTests[i])
    if (Results[i] == 0):
        print("\t'%s'%s - PASSED ID: %d - 0" % (UnitTests[i], " "*DupLen, i))

print("====== FAILED RESULTS ======")
for i in range(UnitTestsSize):
    DupLen = MaxFileNameStringLen - len(UnitTests[i])
    if (Results[i] != 0):
        print("\t'%s'%s - FAILED ID: %d - %s" % (UnitTests[i], " "*DupLen, i, hex(Results[i])))
