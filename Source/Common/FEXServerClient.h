#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/UContext.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <string>
#include <vector>

namespace FEXServerClient {
  enum class PacketType {
    // Request and Result
    TYPE_KILL,
    TYPE_GET_LOG_FD,
    TYPE_GET_ROOTFS_PATH,
    TYPE_GET_PID_FD,
    TYPE_GET_COREDUMP_FD,

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

  std::string GetServerLockFolder();
  std::string GetServerLockFile();
  std::string GetServerRootFSLockFile();
  std::string GetServerMountFolder();
  std::string GetServerSocketName();
  int GetServerFD();

  bool SetupClient(char *InterpreterPath);

  /**
   * @brief Connect to and start a FEXServer instance if required
   *
   * @return socket FD for communicating with server
   */
  int ConnectToAndStartServer(char *InterpreterPath);

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

  std::string RequestRootFSPath(int ServerSocket);

  /**
   * @brief Request a FEXServer to give us a pidfd of the process
   *
   * @param ServerSocket - Socket to the server
   *
   * @return FD for pidfd
   */
  int RequestPIDFD(int ServerSocket);

  /**
   * @brief Request a FEXServer to give us a log FD to communicate with the coredump service
   *
   * @param ServerSocket - Socket to the server
   *
   * @return FD for coredump communication
   */
  int RequestCoredumpFD(int ServerSocket);

  /**  @} */

  namespace Socket {
    size_t ReadDataFromSocket(int Socket, std::vector<uint8_t> *Data);
  }
  /**
   * @name FEX logging through FEXServer
   * @{ */
  namespace Logging {
    enum class PacketTypes : uint32_t {
      TYPE_MSG,
    };

    struct PacketHeader {
      uint64_t Timestamp{};
      PacketTypes PacketType{};
      int32_t PID{};
      int32_t TID{};
      uint32_t Pad{};
      char Data[0];
    };

    struct PacketMsg {
      PacketHeader Header{};
      size_t MessageLength;
      uint32_t Level{};
    };

    static_assert(sizeof(PacketHeader) == 24, "Wrong size");

    [[maybe_unused]]
    static PacketHeader FillHeader(Logging::PacketTypes Type) {
      struct timespec Time{};
      uint64_t Timestamp{};
      clock_gettime(CLOCK_MONOTONIC, &Time);
      Timestamp = Time.tv_sec * 1e9 + Time.tv_nsec;

      Logging::PacketHeader Msg {
        .Timestamp = Timestamp,
        .PacketType = Type,
        .PID = ::getpid(),
        .TID = FHU::Syscalls::gettid(),
      };

      return Msg;
    }
  }

  void MsgHandler(int FD, LogMan::DebugLevels Level, char const *Message);
  void AssertHandler(int FD, char const *Message);
  /**  @} */

  /**
   * @name FEX core dump through FEXServer
   * @{ */
  namespace CoreDump {
    enum class PacketTypes : uint32_t {
      DESC,
      FD_COMMANDLINE,
      FD_MAPS,
      FD_MAP_FILES,
      CLIENT_SHUTDOWN,
      HOST_CONTEXT,
      GUEST_CONTEXT,
      CONTEXT_UNWIND,
      PEEK_MEMORY,
      PEEK_MEMORY_RESPONSE,
      GET_FD_FROM_CLIENT,
      GET_JIT_REGIONS,

      ACK,
      SUCCESS,
    };

    struct PacketHeader {
      PacketTypes PacketType{};
    };

    inline PacketHeader FillHeader(CoreDump::PacketTypes Type) {
      return PacketHeader {
        .PacketType = Type,
      };
    }

    struct PacketDescription {
      PacketHeader Header{};
      uint32_t pid{};
      uint32_t tid{};
      uint32_t uid{};
      uint32_t gid{};
      uint32_t Signal;
      uint64_t Timestamp{};
      uint8_t HostArch;
      uint8_t GuestArch;

      static PacketDescription Fill(uint32_t Signal, uint8_t GuestArch) {
        uint64_t Timestamp{};
        time_t now = time(nullptr);
        Timestamp = now;

        return PacketDescription {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::DESC),
          .pid = (uint32_t)::getpid(),
          .tid = (uint32_t)::gettid(),
          .uid = ::getuid(),
          .gid = ::getgid(),
          .Signal = Signal,
          .Timestamp = Timestamp,
#ifdef _M_X86_64
          .HostArch = 1,
#elif defined(_M_ARM_64)
          .HostArch = 2,
#else
#error unknown architecture
#endif
          .GuestArch = GuestArch,
        };
      }
    };

    struct PacketHostContext {
      PacketHeader Header{};
      siginfo_t siginfo;
      mcontext_t context;

      static PacketHostContext Fill(siginfo_t const *siginfo, mcontext_t const *context) {
        return PacketHostContext {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::HOST_CONTEXT),
          .siginfo = *siginfo,
          .context = *context,
        };
      }
    };

    struct PacketGuestContext {
      PacketHeader Header{};
      siginfo_t siginfo;
      FEXCore::x86_64::mcontext_t context;
      uint8_t GuestArch;

      static PacketGuestContext Fill(siginfo_t const *siginfo, FEXCore::x86_64::mcontext_t const *context, uint8_t GuestArch) {
        return PacketGuestContext {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GUEST_CONTEXT),
          .siginfo = *siginfo,
          .context = *context,
          .GuestArch = GuestArch,
        };
      }
    };

    struct PacketPeekMem {
      PacketHeader Header{};
      uint64_t Addr;
      uint32_t Size;
      static PacketPeekMem Fill(uint64_t Addr, uint32_t Size) {
        return PacketPeekMem {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::PEEK_MEMORY),
          .Addr = Addr,
          .Size = Size,
        };
      }
    };

    struct PacketPeekMemResponse {
      PacketHeader Header{};
      uint64_t Data;
      static PacketPeekMemResponse Fill(uint64_t Data) {
        return PacketPeekMemResponse {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::PEEK_MEMORY_RESPONSE),
          .Data = Data,
        };
      }
    };

    struct PacketGetFDFromFilename {
      PacketHeader Header{};
      size_t FilenameLength;
      char Filepath[];
      static PacketGetFDFromFilename Fill(std::string const *Filename) {
        return PacketGetFDFromFilename {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GET_FD_FROM_CLIENT),
          .FilenameLength = Filename->size(),
        };
      }
    };

    struct PacketGetJITRegions {
      PacketHeader Header{};
      FEXCore::Context::Context::JITRegionPairs Dispatcher{};
      uint64_t NumJITRegions{};
      FEXCore::Context::Context::JITRegionPairs JITRegions[];
      static PacketGetJITRegions Fill(FEXCore::Context::Context::JITRegionPairs const *Dispatcher, uint64_t NumJITRegions) {
        return PacketGetJITRegions {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GET_JIT_REGIONS),
          .Dispatcher = *Dispatcher,
          .NumJITRegions = NumJITRegions,
        };
      }
    };

    void SendDescPacket(int CoreDumpSocket, uint32_t Signal, uint8_t GuestArch);
    void SendCommandLineFD(int CoreDumpSocket);
    void SendMapsFD(int CoreDumpSocket);
    void SendMapFilesFD(int CoreDumpSocket);
    void SendHostContext(int CoreDumpSocket, siginfo_t const *siginfo, mcontext_t const *context);
    void SendGuestContext(int CoreDumpSocket, siginfo_t const *siginfo, FEXCore::x86_64::mcontext_t const *context, uint8_t GuestArch);
    void SendContextUnwind(int CoreDumpSocket);
    void SendJITRegions(int CoreDumpSocket, FEXCore::Context::Context::JITRegionPairs const *Dispatcher, std::vector<FEXCore::Context::Context::JITRegionPairs> const *RegionPairs);
    void WaitForRequests(int CoreDumpSocket);
  }
  /**  @} */

}
