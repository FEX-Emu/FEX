#pragma once

#include <FEXCore/Core/Context.h>

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <optional>
#include <span>
#include <sys/resource.h>
#include <unistd.h>

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
      PacketType Type{};
    } Header;

    struct {
      struct Header Header{};
    } BasicRequest;
  };

  union FEXServerResultPacket {
    struct Header {
      PacketType Type{};
    } Header;

    struct {
      struct Header Header{};
      int32_t PID{};
    } PID;

    struct {
      struct Header Header{};
      size_t Length{};
      char Mount[];
    } MountPath;
  };

  constexpr size_t MAXIMUM_REQUEST_PACKET_SIZE = sizeof(FEXServerRequestPacket);

  fextl::string GetServerLockFolder();
  fextl::string GetServerLockFile();
  fextl::string GetServerRootFSLockFile();
  fextl::string GetServerMountFolder();
  fextl::string GetServerSocketName();
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
   * @brief Request a FEXServer to give us a log FD to communicate with the coredump service
   *
   * @param ServerSocket - Socket to the server
   *
   * @return FD for coredump communication
   */
  int RequestCoredumpFD(int ServerSocket);

  /**
   * @brief Sends an error packet through the socket.
   *
   * @param ServerSocket - Socket to the server.
   */
  void SendEmptyErrorPacket(int ServerSocket);

  /**
   * @brief Sends an FD through the socket.
   *
   * @param ServerSocket - Socket to the server.
   * @param Msg - The packet data to send.
   * @param Size - The size of the packet data.
   * @param FD - The FD to send with the packet.
   *
   * @return Number of bytes sent or -1 on error.
   */
  ssize_t SendFDPacket(int ServerSocket, void *Msg, size_t Size, int FD);
  /**  @} */

  namespace Socket {
    size_t ReadDataFromSocket(int Socket, fextl::vector<uint8_t> *Data);
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
      uint32_t Pad{};
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
      APPLICATIONNAME,
      FD_COMMANDLINE,
      FD_MAPS,
      FD_MAP_FILES,
      FD_COREDUMP_FILTER,
      CLIENT_SHUTDOWN,
      HOST_CONTEXT,
      GUEST_CONTEXT,
      GUEST_XSTATE,
      GUEST_AUXV,
      CONTEXT_UNWIND,
      PEEK_MEMORY,
      PEEK_MEMORY_RESPONSE,
      READ_MEMORY,
      READ_MEMORY_RESPONSE,
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
      uint8_t HostArch{};
      uint8_t GuestArch{};

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

    struct PacketApplicationName {
      PacketHeader Header{};
      size_t FilenameLength{};
      char Filepath[];
      static PacketApplicationName Fill(std::string_view const Filename) {
        return PacketApplicationName {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::APPLICATIONNAME),
          .FilenameLength = Filename.size(),
        };
      }
    };

    struct PacketHostContext {
      PacketHeader Header{};
      siginfo_t siginfo{};
      mcontext_t context{};

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
      uint64_t pr_sigpend{};
      uint64_t pr_sighold{};
      uint32_t pr_ppid{};
      uint32_t pr_pgrp{};
      uint32_t pr_sid{};
      struct timeval pr_utime{};
      struct timeval pr_stime{};
      struct timeval pr_cutime{};
      struct timeval pr_cstime{};

      siginfo_t siginfo{};
      uint8_t OpaqueContext[256]{};
      uint8_t GuestArch{};

      static PacketGuestContext Fill(siginfo_t const *siginfo, void const *context, size_t ContextSize, uint8_t GuestArch) {
        LOGMAN_THROW_A_FMT(sizeof(OpaqueContext) >= ContextSize, "Context size grew too large");
        PacketGuestContext Msg {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GUEST_CONTEXT),
          .siginfo = *siginfo,
          .GuestArch = GuestArch,
        };
        auto SetMask = [](sigset_t &Set) -> uint64_t {
          uint64_t Mask{};
          for (size_t i = 0; i < 64; ++i) {
            if (sigismember(&Set, i)) {
              Mask |= (1ULL << i);
            }
          }
          return Mask;
        };

        sigset_t SigSet{};
        sigpending(&SigSet);
        Msg.pr_sigpend = SetMask(SigSet);
        sigprocmask(SIG_SETMASK, nullptr, &SigSet);
        Msg.pr_sighold = SetMask(SigSet);
        Msg.pr_ppid = ::getppid();
        Msg.pr_pgrp = ::getpgrp();
        Msg.pr_sid = ::getsid(0);

        struct rusage usage{};
        getrusage(RUSAGE_THREAD, &usage);
        Msg.pr_utime = usage.ru_utime;
        Msg.pr_stime = usage.ru_stime;
        getrusage(RUSAGE_CHILDREN, &usage);
        Msg.pr_cutime = usage.ru_utime;
        Msg.pr_cstime = usage.ru_stime;

        memcpy(&Msg.OpaqueContext, context, ContextSize);

        return Msg;
      }
    };

    struct PacketGuestXState {
      PacketHeader Header{};
      uint8_t AVX{};
      uint8_t OpaqueState[944]{};

      static PacketGuestXState Fill(bool AVX, void const *xstate, size_t XStateSize) {
        LOGMAN_THROW_A_FMT(sizeof(OpaqueState) >= XStateSize, "XState size grew too large");
        PacketGuestXState Msg {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GUEST_XSTATE),
          .AVX = AVX,
        };
        memcpy(&Msg.OpaqueState, xstate, XStateSize);

        return Msg;
      }
    };

    struct PacketGuestAuxv {
      PacketHeader Header{};
      uint64_t AuxvSize{};
      uint8_t Auxv[];
      static PacketGuestAuxv Fill(uint64_t AuxvSize) {
        return PacketGuestAuxv {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GUEST_AUXV),
          .AuxvSize = AuxvSize,
        };
      }
    };

    struct PacketPeekMem {
      PacketHeader Header{};
      uint64_t Addr{};
      uint32_t Size{};
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
      uint64_t Data{};
      static PacketPeekMemResponse Fill(uint64_t Data) {
        return PacketPeekMemResponse {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::PEEK_MEMORY_RESPONSE),
          .Data = Data,
        };
      }
    };

    struct PacketReadMem {
      PacketHeader Header{};
      uint64_t Addr{};
      uint32_t Size{};
      static PacketReadMem Fill(uint64_t Addr, uint32_t Size) {
        return PacketReadMem {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::READ_MEMORY),
          .Addr = Addr,
          .Size = Size,
        };
      }
    };

    struct PacketReadMemResponse {
      PacketHeader Header{};
      bool HadError{};
      static PacketReadMemResponse Fill(bool HadError) {
        return PacketReadMemResponse {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::READ_MEMORY_RESPONSE),
          .HadError = HadError,
        };
      }
    };

    struct PacketGetFDFromFilename {
      PacketHeader Header{};
      size_t FilenameLength{};
      char Filepath[];
      static PacketGetFDFromFilename Fill(std::string_view const Filename) {
        return PacketGetFDFromFilename {
          .Header = CoreDump::FillHeader(CoreDump::PacketTypes::GET_FD_FROM_CLIENT),
          .FilenameLength = Filename.size() + 1,
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

    ssize_t SendPacketWithData(int Socket, void *Msg, size_t size, std::span<const char> Data);

    void SendAckPacket(int CoreDumpSocket);

    ///< Client side handling.
    void SendDescPacket(int CoreDumpSocket, uint32_t Signal, uint8_t GuestArch);
    void SendApplicationName(int CoreDumpSocket, std::string_view ApplicationName);
    void SendCommandLineFD(int CoreDumpSocket);
    void SendMapsFD(int CoreDumpSocket);
    void SendCoreDumpFilter(int CoreDumpSocket);
    void SendMapFilesFD(int CoreDumpSocket);
    void SendHostContext(int CoreDumpSocket, siginfo_t const *siginfo, mcontext_t const *context);
    void SendGuestContext(int CoreDumpSocket, siginfo_t const *siginfo, void const *context, size_t ContextSize, uint8_t GuestArch);
    void SendGuestXState(int CoreDumpSocket, bool AVX, void const *xstate, size_t XStateSize);
    void SendGuestAuxv(int CoreDumpSocket, const void *auxv, uint64_t AuxvSize);
    void SendContextUnwind(int CoreDumpSocket);
    void SendJITRegions(int CoreDumpSocket, FEXCore::Context::Context::JITRegionPairs const *Dispatcher, fextl::vector<FEXCore::Context::Context::JITRegionPairs> const *RegionPairs);

    void WaitForRequests(int CoreDumpSocket);

    ///< Server side handling.
    int GetFDFromClient(int ServerSocket, std::string_view const Filename);
    void SendShutdownPacket(int CoreDumpSocket);
    std::optional<uint64_t> PeekMemory(int ServerSocket, uint64_t Addr, uint32_t Size);
    bool ReadMemory(int ServerSocket, void *ResultMemory, uint64_t Addr, uint32_t Size);
    int HandleFDPacket(int CoreDumpSocket);
  }
  /**  @} */

}
