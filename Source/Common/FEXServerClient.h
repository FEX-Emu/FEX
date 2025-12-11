// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/string.h>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string_view>

namespace LogMan {
enum DebugLevels : uint32_t;
}

namespace FEXServerClient {
enum class PacketType {
  // Request and Result
  TYPE_KILL,
  TYPE_GET_LOG_FD,
  TYPE_GET_ROOTFS_PATH,
  TYPE_GET_PID_FD,
  TYPE_POPULATE_CODE_CACHE,
  TYPE_POPULATE_CODE_CACHE_NO_MULTIBLOCK,
  TYPE_QUERY_CODE_MAP,
  TYPE_QUERY_CODE_MAP_NO_MULTIBLOCK,

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
fextl::string GetServerSocketPath();
int GetServerFD();

bool SetupClient(std::string_view InterpreterPath);

/**
 * @brief Start a FEXServer instance if possible
 *
 * @return socket FD for communicating with server
 */
int StartServer(std::string_view InterpreterPath, int watch_fd = -1);

/**
 * @brief Connect to and start a FEXServer instance if required
 *
 * @return socket FD for communicating with server
 */
int ConnectToAndStartServer(std::string_view InterpreterPath);

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

/**
 * @brief Request FEXServer to populate the disk cache for the given executable
 *        and any libraries referenced in its code map
 *
 * @param ServerSocket - Socket to the server
 * @param ProgramFD - FD for program binary
 * @param HasMultiblock - true if multiblock is enabled (used for selecting code maps)
 */
void PopulateCodeCache(int ServerSocket, int ProgramFD, bool HasMultiblock);

/**
 * @brief Request FEXServer to create a new code map for disk cache population
 *
 * @param ServerSocket - Socket to the server
 * @param ProgramFD - FD for program binary
 *
 * @return FD to write code map to
 */
int RequestCodeMapFD(int ServerSocket, int ProgramFD, bool HasMultiblock);

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

  PacketHeader FillHeader(PacketTypes Type);
} // namespace Logging

void MsgHandler(int FD, LogMan::DebugLevels Level, const char* Message);
void AssertHandler(int FD, const char* Message);
/**  @} */
} // namespace FEXServerClient
