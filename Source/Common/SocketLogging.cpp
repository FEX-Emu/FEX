#include "Common/SocketLogging.h"

#include <FEXCore/Utils/NetStream.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <atomic>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace FEX::SocketLogging {
  namespace Common {
    enum class PacketTypes : uint32_t {
      TYPE_MSG,
      TYPE_ACK,
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
      uint32_t Level{};
      char Msg[0];
    };

    static_assert(sizeof(PacketHeader) == 24, "Wrong size");

    static PacketHeader FillHeader(Common::PacketTypes Type) {
      struct timespec Time{};
      uint64_t Timestamp{};
      clock_gettime(CLOCK_MONOTONIC, &Time);
      Timestamp = Time.tv_sec * 1e9 + Time.tv_nsec;

      Common::PacketHeader Msg {
        .Timestamp = Timestamp,
        .PacketType = Type,
        .PID = ::getpid(),
        .TID = FHU::Syscalls::gettid(),
      };

      return Msg;
    }
  }

  namespace Client {
    class ClientConnector {
      public:
        ClientConnector(int FD)
          : Socket {std::make_unique<FEXCore::Utils::NetStream>(FD)} {
        }
        void MsgHandler(LogMan::DebugLevels Level, bool Synchronize, char const *Message);
        void AssertHandler(char const *Message) {
          MsgHandler(LogMan::DebugLevels::ASSERT, true, Message);
        }

      private:
        std::unique_ptr<std::iostream> Socket;
    };

    void ClientConnector::MsgHandler(LogMan::DebugLevels Level, bool Synchronize, char const *Message) {
      Common::PacketMsg Msg {
        .Header = Common::FillHeader(Common::PacketTypes::TYPE_MSG),
        .Level = Level,
      };
      size_t MsgLen = strlen(Message) + 1;
      size_t PacketSize = sizeof(Common::PacketMsg) + MsgLen;

      // XXX: Alloca for small packets?
      Common::PacketMsg *MsgP = reinterpret_cast<Common::PacketMsg*>(malloc(PacketSize));
      memcpy(MsgP, &Msg, sizeof(Common::PacketMsg));
      memcpy(MsgP->Msg, Message, MsgLen);
      // First write the Packet
      Socket->write(reinterpret_cast<const char*>(MsgP), PacketSize);
      // Now flush it
      Socket->flush();

      if (Synchronize) {
        auto Ack = FillHeader(Common::PacketTypes::TYPE_ACK);
        // First write the Packet
        Socket->write(reinterpret_cast<const char*>(&Ack), sizeof(Ack));
        // Now flush it
        Socket->flush();

        if (Socket->read(reinterpret_cast<char*>(&Ack), sizeof(Ack))) {
          if (Ack.PacketType == Common::PacketTypes::TYPE_ACK) {
            // This is what was expected
          }
        }
      }

      free(MsgP);
    }

    static std::unique_ptr<ClientConnector> Client{};

    void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
      Client->MsgHandler(Level, false, Message);
    }

    void AssertHandler(char const *Message) {
      Client->AssertHandler(Message);
    }

    bool ConnectToClient(const std::string &Remote) {
      // Time to open up the actual socket and send the FD over to the daemon
      // Create the initial unix socket
      int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (socket_fd == -1) {
        return false;
      }

      // XXX: Not parsing Remote atm
      struct hostent *Entry{};
      std::string Host{};
      uint32_t Port{8087};

      std::istringstream Input {Remote};
      std::string Line;
      std::getline(Input, Host, ':');
      if (std::getline(Input, Line, ':')) {
        Port = std::stoi(Line);
      }

      if ((Entry = gethostbyname(Host.c_str())) == nullptr) {
        perror("gethostbyname");
        return false;
      }

      struct sockaddr_in addr{};
      memcpy(&addr.sin_addr, Entry->h_addr_list[0], Entry->h_length);
      addr.sin_family = AF_INET;
      addr.sin_port = htons(Port);

      if (connect(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        close(socket_fd);
        return false;
      }

      Client = std::make_unique<ClientConnector>(socket_fd);
      return true;
    }
  }

  namespace Server {
    static void* ThreadHandler(void *Arg);
    class ServerListenerImpl final : public ServerListener {
      public:
        ServerListenerImpl(std::string_view Socket) {
          OpenListenSocket(Socket);

          uint64_t OldMask = FEXCore::Threads::SetSignalMask(~0ULL);
          ListenThread = FEXCore::Threads::Thread::Create(ThreadHandler, this);
          FEXCore::Threads::SetSignalMask(OldMask);
        }

        ~ServerListenerImpl() {
          ShuttingDown = true;
          close(ListenSocket);

          // Wait for the thread to leave
          ListenThread->join(nullptr);
        }

        void ListenThreadFunc() {
          while (!ShuttingDown.load()) {
            // Wait for new data to come in
            // Wait for ten seconds
            struct timespec ts{};
            ts.tv_sec = 10;

            int Result = ppoll(&PollFDs.at(0), PollFDs.size(), &ts, nullptr);
            if (Result > 0) {
              // Walk the FDs and see if we got any results
              for (auto it = PollFDs.begin(); it != PollFDs.end(); ) {
                auto &Event = *it;
                bool Erase{};

                if (Event.revents != 0) {
                  if (Event.fd == ListenSocket) {
                    if (Event.revents & POLLIN) {
                      // If it is the listen socket then we have a new connection
                      struct sockaddr_storage Addr{};
                      socklen_t AddrSize{};
                      int NewFD = accept(ListenSocket, reinterpret_cast<struct sockaddr*>(&Addr), &AddrSize);

                      // Add the new client to the array
                      PollFDs.emplace_back(pollfd {
                        .fd = NewFD,
                        .events = POLLIN | POLLPRI | POLLRDHUP | POLLREMOVE,
                        .revents = 0,
                      });

                      // Set to nonblocking
                      int Flags = fcntl(NewFD, F_GETFL);
                      fcntl(NewFD, F_SETFL, Flags | O_NONBLOCK);
                    }
                    else if (Event.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                      // Listen socket error or shutting down
                      break;
                    }
                  }
                  else {
                    if (Event.revents & POLLIN) {
                      // Data from the socket
                      HandleSocketData(Event.fd);
                    }

                    if (Event.revents & (POLLHUP | POLLERR | POLLNVAL | POLLRDHUP)) {
                      // Error or hangup, close the socket and erase it from our list
                      Erase = true;
                      close(Event.fd);
                      ClosedHandler(Event.fd);
                    }
                  }
                  Event.revents = 0;
                  --Result;
                }

                if (Erase) {
                  it = PollFDs.erase(it);
                }
                else {
                  ++it;
                }

                if (Result == 0) {
                  // Early break if we've consumed all the results
                  break;
                }
              }
            }
            else {
              // EINTR is common here
            }
          }

          // Walk the socket list and close everything
          for (auto &Event : PollFDs) {
            close(Event.fd);
            ClosedHandler(Event.fd);
          }
          PollFDs.clear();

          ShutdownEvent.NotifyAll();
        }

      private:
        std::unique_ptr<FEXCore::Threads::Thread> ListenThread{};
        std::atomic_bool ShuttingDown{};
        int ListenSocket{-1};
        std::vector<struct pollfd> PollFDs{};

        void OpenListenSocket(std::string_view Socket) {
          struct addrinfo Hints{};
          struct addrinfo *Result{};

          Hints.ai_family = AF_INET;
          Hints.ai_socktype = SOCK_STREAM;
          Hints.ai_flags = AI_PASSIVE;

          std::string Port = std::string(Socket);
          if (getaddrinfo(nullptr, Port.c_str(), &Hints, &Result) < 0) {
            perror("getaddrinfo");
          }

          ListenSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
          if (ListenSocket < 0) {
            perror("socket");
          }

          int on = 1;
          if (setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0) {
            perror("setsockopt");
            close(ListenSocket);
            ListenSocket = -1;
          }

          if (bind(ListenSocket, Result->ai_addr, Result->ai_addrlen) < 0) {
            perror("bind");
            close(ListenSocket);
            ListenSocket = -1;
          }

          listen(ListenSocket, 16);
          PollFDs.emplace_back(pollfd {
            .fd = ListenSocket,
            .events = POLLIN,
            .revents = 0,
          });
        }

        void HandleSocketData(int Socket) {
          std::vector<uint8_t> Data(1500);
          size_t CurrentRead{};
          while (true) {
            int Read = read(Socket, &Data.at(CurrentRead), Data.size() - CurrentRead);
            if (Read > 0) {
              CurrentRead += Read;
              if (CurrentRead == Data.size()) {
                Data.resize(Data.size() << 1);
              }
            }
            else if (Read == 0) {
              // No more to read
              break;
            }
            else {
              if (errno == EWOULDBLOCK) {
                // no error
              }
              else {
                perror("read");
              }
              break;
            }
          }

          size_t CurrentOffset{};
          while (CurrentOffset < CurrentRead) {
            Common::PacketHeader *Header = reinterpret_cast<Common::PacketHeader*>(&Data[CurrentOffset]);
            if (Header->PacketType == Common::PacketTypes::TYPE_MSG) {
              Common::PacketMsg *Msg = reinterpret_cast<Common::PacketMsg*>(&Data[0]);

              MsgHandler(Socket, Msg->Header.Timestamp, Msg->Header.PID, Msg->Header.TID, Msg->Level, Msg->Msg);
              CurrentOffset += sizeof(Common::PacketMsg) + strlen(Msg->Msg) + 1;
            }
            else if (Header->PacketType == Common::PacketTypes::TYPE_ACK) {
              // If the client sent an ACK then we want to send one right back
              auto Ack = FillHeader(Common::PacketTypes::TYPE_ACK);
              write(Socket, &Ack, sizeof(Ack));
              CurrentOffset += sizeof(Ack);
            }
            else {
              CurrentOffset = CurrentRead;
            }
          }
        }
    };

    static void* ThreadHandler(void *Arg) {
      ServerListenerImpl *This = reinterpret_cast<ServerListenerImpl*>(Arg);
      This->ListenThreadFunc();
      return nullptr;
    }

    std::unique_ptr<ServerListener> StartListening(std::string_view Socket) {
      return std::make_unique<ServerListenerImpl>(Socket);
    }
  }
}

