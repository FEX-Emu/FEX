#!/usr/bin/python3
import os
import shutil
import subprocess
import sys

import xxhash


def GetDistroInfo():
    DistroName = "Unknown"
    DistroVersion = "Unknown"

    with open("/etc/lsb-release", "r") as f:
        while True:
            Line = f.readline()
            if not Line:
                break
            Split = Line.split("=")
            if Split[0] == "DISTRIB_ID":
                DistroName = Split[1].lower().rstrip()
            if Split[0] == "DISTRIB_RELEASE":
                DistroVersion = Split[1].rstrip()

    return [DistroName, DistroVersion]


def FindBestImageFit(Distro, links_file):
    CurrentFitSize = 0
    BestFitDistro = None
    BestFitDistroVersion = None
    BestFitReadableName = None
    BestFitImagePath = None
    BestFitHash = None

    with open(links_file, "r") as f:
        while True:
            # Order:
            # Distro Name
            # Distro Version
            # User readable name
            # File Path
            # Hash

            DistroName = f.readline().strip()
            if not DistroName:
                break

            DistroVersion = f.readline().strip()
            DistroReadableName = f.readline().strip()
            DistroImagePath = f.readline().strip()
            DistroHash = f.readline().strip()

            FitRate = 0
            if DistroName == Distro[0] or DistroName == None:
                FitRate += 1

            if DistroVersion == Distro[1] or DistroVersion == None:
                FitRate += 1

            if FitRate > CurrentFitSize:
                CurrentFitSize = FitRate
                BestFitDistro = DistroName
                BestFitDistroVersion = DistroVersion
                BestFitReadableName = DistroReadableName
                BestFitImagePath = DistroImagePath
                BestFitHash = DistroHash

    return [
        BestFitDistro,
        BestFitDistroVersion,
        BestFitReadableName,
        BestFitImagePath,
        int(BestFitHash, 16),
    ]


def HashFile(file):
    # 32MB buffer size
    BUFFER_SIZE = 32 * 1024 * 1024

    x = xxhash.xxh3_64(seed=0)
    b = bytearray(BUFFER_SIZE)
    mv = memoryview(b)

    with open(file, "rb") as f:
        while n := f.readinto(mv):
            x.update(mv[:n])

    return int.from_bytes(x.digest(), "big")


def RemoveRootFSFolder(RootFSPath):
    print("Removing previous rootfs extraction before copying")
    shutil.rmtree(RootFSPath, ignore_errors=True)
    # Recreate the folder
    os.makedirs(RootFSPath)


def CheckFilesystemForFS(RootFSMountPath, RootFSPath, DistroFit):
    # Check if rootfs mount path exists
    if not os.path.exists(RootFSMountPath) or not os.path.isdir(RootFSMountPath):
        print("RootFS mount path is wrong")
        return False

    # Check if rootfs path exists
    if not os.path.exists(RootFSPath) or not os.path.isdir(RootFSPath):
        # Create this directory
        os.makedirs(RootFSPath)

    # Check if rootfs path exists
    if not os.path.isdir(RootFSPath):
        print("RootFS path is not a directory")
        return False

    # Check rootfs folder for image, copy and extract as necessary
    MountRootFSImagePath = RootFSMountPath + DistroFit[3]
    RootFSImagePath = RootFSPath + "/" + os.path.basename(DistroFit[3])
    NeedsExtraction = False
    PreviouslyExistingRootFS = False

    if not os.path.exists(MountRootFSImagePath):
        print("Image {} doesn't exist".format(MountRootFSImagePath))
        return False

    if not os.path.exists(RootFSImagePath):
        # Copy over
        print("RootFS image doesn't exist. Copying")
        RemoveRootFSFolder(RootFSPath)
        shutil.copyfile(MountRootFSImagePath, RootFSImagePath)
        NeedsExtraction = True

    # Check if the image needs to be extracted
    if not os.path.exists(RootFSPath + "/usr"):
        NeedsExtraction = True
    else:
        PreviouslyExistingRootFS = True

    # Now hash the image
    RootFSHash = HashFile(RootFSImagePath)
    if RootFSHash != DistroFit[4]:
        print(
            "Hash {} did not match {}, copying new image".format(
                hex(RootFSHash), hex(DistroFit[4])
            )
        )

        if PreviouslyExistingRootFS:
            RemoveRootFSFolder(RootFSPath)

        shutil.copyfile(MountRootFSImagePath, RootFSImagePath)
        NeedsExtraction = True

    if NeedsExtraction:
        print("Extracting rootfs")

        CmdResult = subprocess.call(
            ["unsquashfs", "-f", "-d", RootFSPath, RootFSImagePath]
        )
        if CmdResult != 0:
            print("Couldn't extract squashfs. Removing image file to be safe")
            os.remove(RootFSImagePath)
            return False

    if not os.path.exists(RootFSPath + "/usr"):
        print("Couldn't extract squashfs. Removing image file to be safe")
        os.remove(RootFSImagePath)
        return False

    print("RootFS successfully checked and extracted")

    return True


def main():
    if sys.version_info[0] < 3:
        logging.critical("Python 3 or a more recent version is required.")

    FEX_ROOTFS_MOUNT = os.getenv("FEX_ROOTFS_MOUNT")
    FEX_ROOTFS_PATH = os.getenv("FEX_ROOTFS_PATH")

    if FEX_ROOTFS_MOUNT == None:
        print("Need FEX_ROOTFS_MOUNT set")
        sys.exit(1)

    if FEX_ROOTFS_PATH == None:
        print("Need FEX_ROOTFS_PATH set")
        sys.exit(1)

    if shutil.which("unsquashfs") is None:
        print("CI system didn't have unsquashfs installed")
        sys.exit(1)

    Distro = GetDistroInfo()
    DistroFit = FindBestImageFit(Distro, FEX_ROOTFS_MOUNT + "/RootFS_links.txt")

    if CheckFilesystemForFS(FEX_ROOTFS_MOUNT, FEX_ROOTFS_PATH, DistroFit) == False:
        print("Couldn't load filesystem rootfs")
        sys.exit(1)

    return 0


if __name__ == "__main__":
    # execute only if run as a script
    sys.exit(main())
