// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/string.h>
#include <FEXHeaderUtils/Syscalls.h>

namespace FEXServerClient {
enum class PacketType {
  // Request and Result
  TYPE_KILL,
  TYPE_GET_LOG_FD,
  TYPE_GET_ROOTFS_PATH,
  TYPE_GET_PID_FD,

  // Result only
  TYPE_SUCCESS,
  TYPE_ERROR,
};

union FEXServerRequestPacket {
  struct Header {
    PacketType Type;
  } Header;

  struct {
    struct Header Header;
  } BasicRequest;
};

union FEXServerResultPacket {
  struct Header {
    PacketType Type;
  } Header;

  struct {
    struct Header Header;
    int32_t PID;
  } PID;

  struct {
    struct Header Header;
    size_t Length;
    char Mount[0];
  } MountPath;
};

constexpr size_t MAXIMUM_REQUEST_PACKET_SIZE = sizeof(FEXServerRequestPacket);

fextl::string GetServerLockFolder();
fextl::string GetServerLockFile();
fextl::string GetServerRootFSLockFile();
fextl::string GetTempFolder();
fextl::string GetServerMountFolder();
fextl::string GetServerSocketName();
int GetServerFD();

bool SetupClient(char* InterpreterPath);

/**
 * @brief Connect to and start a FEXServer instance if required
 *
 * @return socket FD for communicating with server
 */
int ConnectToAndStartServer(char* InterpreterPath);

enum class ConnectionOption {
  Default,
  NoPrintConnectionError,
};
/**
 * @brief Connect to a FEXServer instance if it exists
 *
 * @return socket FD for communicating with server
 */
int ConnectToServer(ConnectionOption ConnectionOption = ConnectionOption::Default);

/**
 * @name Packet request functions
 * @{ */
/**
 * @brief Request the server to be killed
 *
 * @param ServerSocket - Socket to the server
 */
void RequestServerKill(int ServerSocket);

/**
 * @brief Request a FEXServer to give us a log FD to write in to
 *
 * @param ServerSocket - Socket to the server
 *
 * @return FD for logging in to
 */
int RequestLogFD(int ServerSocket);

fextl::string RequestRootFSPath(int ServerSocket);

/**
 * @brief Request a FEXServer to give us a pidfd of the process
 *
 * @param ServerSocket - Socket to the server
 *
 * @return FD for pidfd
 */
int RequestPIDFD(int ServerSocket);

/**  @} */

/**
 * @name FEX logging through FEXServer
 * @{ */
namespace Logging {
  enum class PacketTypes : uint32_t {
    TYPE_MSG,
  };

  struct PacketHeader {
    struct timespec Timestamp {};
    PacketTypes PacketType {};
    int32_t PID {};
    int32_t TID {};
    uint32_t Pad {};
    char Data[0];
  };

  struct PacketMsg {
    PacketHeader Header {};
    size_t MessageLength;
    uint32_t Level {};
    uint32_t Pad {};
  };

  static_assert(sizeof(PacketHeader) == 32, "Wrong size");

  [[maybe_unused]]
  static PacketHeader FillHeader(Logging::PacketTypes Type) {

    Logging::PacketHeader Msg {
      .PacketType = Type,
      .PID = ::getpid(),
      .TID = FHU::Syscalls::gettid(),
    };
    clock_gettime(CLOCK_MONOTONIC, &Msg.Timestamp);

    return Msg;
  }
} // namespace Logging

void MsgHandler(int FD, LogMan::DebugLevels Level, const char* Message);
void AssertHandler(int FD, const char* Message);
/**  @} */
} // namespace FEXServerClient
