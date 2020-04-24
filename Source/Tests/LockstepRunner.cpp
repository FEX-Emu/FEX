#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "CommonCore/VMFactory.h"
#include "HarnessHelpers.h"
#include "LogManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Memory/SharedMem.h>

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <vector>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/thread/thread_time.hpp>

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
  const char *CharLevel{nullptr};

  switch (Level) {
  case LogMan::NONE:
    CharLevel = "NONE";
    break;
  case LogMan::ASSERT:
    CharLevel = "ASSERT";
    break;
  case LogMan::ERROR:
    CharLevel = "ERROR";
    break;
  case LogMan::DEBUG:
    CharLevel = "DEBUG";
    break;
  case LogMan::INFO:
    CharLevel = "Info";
    break;
  default:
    CharLevel = "???";
    break;
  }
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
}

class Flag final {
public:
  bool TestAndSet(bool SetValue = true) {
    bool Expected = !SetValue;
    return Value.compare_exchange_strong(Expected, SetValue);
  }

  bool TestAndClear() {
    return TestAndSet(false);
  }

  void Set() {
    Value.store(true);
  }

  bool Load() {
    return Value.load();
  }

private:
  std::atomic_bool Value {false};
};


class IPCEvent final {
private:
/**
 * @brief Literally just an atomic bool that we are using for this class
 */
public:
  IPCEvent(std::string_view Name, bool Client)
    : EventName {Name} {

    using namespace boost::interprocess;
    if (!Client) {
      shared_memory_object::remove(EventName.c_str());
      SHM = std::make_unique<shared_memory_object>(
        create_only,
        EventName.c_str(),
        read_write
      );
      SHMSize = sizeof(SHMObject);
      SHM->truncate(SHMSize);
      SHMRegion = std::make_unique<mapped_region>(*SHM, read_write);

      Obj = new (SHMRegion->get_address()) SHMObject{};
    }
    else {
      SHM = std::make_unique<shared_memory_object>(
        open_only,
        EventName.c_str(),
        read_write
      );
      SHMSize = sizeof(SHMObject);
      SHMRegion = std::make_unique<mapped_region>(*SHM, read_write);

      // Load object from shared memory, don't construct it
      Obj = reinterpret_cast<SHMObject*>(SHMRegion->get_address());
    }
  }

  ~IPCEvent() {
    using namespace boost::interprocess;
    shared_memory_object::remove(EventName.c_str());
  }

  void NotifyOne() {
    using namespace boost::interprocess;
    if (Obj->FlagObject.TestAndSet()) {
      scoped_lock<interprocess_mutex> lk(Obj->MutexObject);
      Obj->CondObject.notify_one();
    }
  }

  void NotifyAll() {
    using namespace boost::interprocess;
    if (Obj->FlagObject.TestAndSet()) {
      scoped_lock<interprocess_mutex> lk(Obj->MutexObject);
      Obj->CondObject.notify_all();
    }
  }

  void Wait() {
    using namespace boost::interprocess;

    // Have we signaled before we started waiting?
    if (Obj->FlagObject.TestAndClear())
      return;

    scoped_lock<interprocess_mutex> lk(Obj->MutexObject);
    Obj->CondObject.wait(lk, [this] { return Obj->FlagObject.TestAndClear(); });
  }

  bool WaitFor(boost::posix_time::ptime const& time) {
    using namespace boost::interprocess;

    // Have we signaled before we started waiting?
    if (Obj->FlagObject.TestAndClear())
      return true;

    scoped_lock<interprocess_mutex> lk(Obj->MutexObject);
    bool DidSignal = Obj->CondObject.timed_wait(lk, time, [this] { return Obj->FlagObject.TestAndClear(); });
    return DidSignal;
  }

private:
  std::string EventName;
  std::unique_ptr<boost::interprocess::shared_memory_object> SHM;
  std::unique_ptr<boost::interprocess::mapped_region> SHMRegion;
  struct SHMObject {
    Flag FlagObject;
    boost::interprocess::interprocess_condition CondObject;
    boost::interprocess::interprocess_mutex MutexObject;
  };
  SHMObject *Obj;
  size_t SHMSize;
};

class IPCFlag {
public:

  IPCFlag(std::string_view Name, bool Client)
    : EventName {Name} {

    using namespace boost::interprocess;
    if (!Client) {
      shared_memory_object::remove(EventName.c_str());
      SHM = std::make_unique<shared_memory_object>(
        create_only,
        EventName.c_str(),
        read_write
      );
      SHMSize = sizeof(FlagObj);
      SHM->truncate(SHMSize);
      SHMRegion = std::make_unique<mapped_region>(*SHM, read_write);

      FlagObj = new (SHMRegion->get_address()) Flag{};
    }
    else {
      SHM = std::make_unique<shared_memory_object>(
        open_only,
        EventName.c_str(),
        read_write
      );
      SHMSize = sizeof(Flag);
      SHMRegion = std::make_unique<mapped_region>(*SHM, read_write);

      // Load object from shared memory, don't construct it
      FlagObj = reinterpret_cast<Flag*>(SHMRegion->get_address());
    }
  }

  ~IPCFlag() {
    using namespace boost::interprocess;
    shared_memory_object::remove(EventName.c_str());
  }

  Flag *GetFlag() { return FlagObj; }

private:
  std::string EventName;
  std::unique_ptr<boost::interprocess::shared_memory_object> SHM;
  std::unique_ptr<boost::interprocess::mapped_region> SHMRegion;
  size_t SHMSize;

  Flag *FlagObj;
};

class IPCMessage {
public:
  IPCMessage(std::string_view Name, bool Client, size_t QueueSize, size_t QueueDepth)
    : QueueName {Name} {
    using namespace boost::interprocess;
    if (!Client) {
      message_queue::remove(QueueName.c_str());
      MessageQueue = std::make_unique<message_queue>(
        create_only,
        QueueName.c_str(),
        QueueDepth,
        QueueSize
      );
    }
    else {
      MessageQueue = std::make_unique<message_queue>(
        open_only,
        QueueName.c_str()
      );
    }
  }

  void Send(void *Buf, size_t Size) {
    MessageQueue->send(Buf, Size, 0);
  }

  size_t Recv(void *Buf, size_t Size) {
    uint32_t Priority {};
    size_t Recv_Size{};
    if (!MessageQueue->timed_receive(Buf, Size, Recv_Size, Priority, boost::get_system_time() + boost::posix_time::seconds(10))) {
      return 0;
    }
    return Recv_Size;
  }

  void ThrowAwayRecv() {
    uint32_t Priority {};
    size_t Recv_Size{};
    MessageQueue->try_receive(nullptr, 0, Recv_Size, Priority);
  }

  ~IPCMessage() {
    using namespace boost::interprocess;
    message_queue::remove(QueueName.c_str());
  }

private:
  std::string QueueName;
  std::unique_ptr<boost::interprocess::message_queue> MessageQueue;
};

/**
 * @brief Heartbeat to know if processes are alive
 */
class IPCHeartbeat {
public:
  IPCHeartbeat(std::string_view Name, bool Client)
    : EventName {Name} {
    using namespace boost::interprocess;

    if (Client) {
      bool Trying = true;
      while (Trying) {
        try {
          SHM = std::make_unique<shared_memory_object>(
            open_only,
            EventName.c_str(),
            read_write
          );

          boost::interprocess::offset_t size;
          while (!SHM->get_size(size)) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

          // Map the SHM region
          // This can fault for...some reason?
          SHMRegion = std::make_unique<mapped_region>(*SHM, read_write);

          HeartBeatEvents = reinterpret_cast<SHMHeartbeatEvent*>(SHMRegion->get_address());
          while (HeartBeatEvents->Allocated.load() == 0)  { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
          Trying = false;
        }
        catch(...) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }

    }
    else {
      SHM = std::make_unique<shared_memory_object>(
        open_or_create,
        EventName.c_str(),
        read_write
      );

      SHM->truncate(sizeof(SHMHeartbeatEvent) + sizeof(HeartbeatClient) * 32);

      // Map the SHM region
      SHMRegion = std::make_unique<mapped_region>(*SHM, read_write);

      HeartBeatEvents = new (SHMRegion->get_address()) SHMHeartbeatEvent{};
      HeartBeatEvents->Allocated = 1;
    }

    ThreadID = AllocateThreadID();
    if (ThreadID == ~0U) {
      Running = false;
      return;
    }
    HeartBeatThread = std::thread(&IPCHeartbeat::Thread, this);
  }

  ~IPCHeartbeat() {
    using namespace boost::interprocess;
    Running = false;
    if (HeartBeatThread.joinable()) {
      HeartBeatThread.join();
    }

    // If we are the last client then we should just destroy this region
    if (DeallocateThreadID(ThreadID)) {
      shared_memory_object::remove(EventName.c_str());
    }
  }

  bool WaitForServer() {
    auto Start = std::chrono::system_clock::now();
    while (!IsServerAlive()) {
      auto Now = std::chrono::system_clock::now();
      using namespace std::chrono_literals;
      if ((Now - Start) >= 10s) {
        // We waited 10 seconds and the server never came up
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
  }

  bool WaitForClient() {
    auto Start = std::chrono::system_clock::now();
    while (!IsClientAlive()) {
      auto Now = std::chrono::system_clock::now();
      using namespace std::chrono_literals;
      if ((Now - Start) >= 10s) {
        // We waited 10 seconds and the server never came up
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
  }


  void Thread() {
    HeartbeatClient *ThisThread = &HeartBeatEvents->Clients[ThreadID];
    while (Running) {
      std::this_thread::sleep_for(HeartBeatRate);
      ThisThread->HeartbeatCounter++;
    }
  }

  /**
   * @brief Once the heartbeat is active we can enable a server flag
   */
  void EnableServer() {
    LogMan::Msg::E("[SERVER %s] Enabling server on thread ID %d", EventName.c_str(), ThreadID);
    HeartbeatClient *ThisThread = &HeartBeatEvents->Clients[ThreadID];
    ThisThread->Type = 1;
  }

  void EnableClient() {
    LogMan::Msg::E("[CLIENT %s] Enabling client on thread ID %d", EventName.c_str(), ThreadID);
    HeartbeatClient *ThisThread = &HeartBeatEvents->Clients[ThreadID];
    ThisThread->Type = 2;
  }

private:

  bool IsServerAlive() {
    return IsTypeAlive(1);
  }

  bool IsClientAlive() {
    return IsTypeAlive(2);
  }

  bool IsTypeAlive(uint32_t Type) {
    uint32_t OriginalThreadMask = HeartBeatEvents->NumClients.load();
    for (uint32_t Index = 0; Index < 32; ++Index) {
      if ((OriginalThreadMask & (1 << Index)) == 0)
        continue;

      HeartbeatClient *ThisThread = &HeartBeatEvents->Clients[Index];
      if (ThisThread->Type == Type) {
        // Found a Server, check if it is live

        uint32_t Counter = ThisThread->HeartbeatCounter.load();
        std::this_thread::sleep_for(HeartBeatRate * 2);
        if (Counter != ThisThread->HeartbeatCounter.load()) {
          return true;
        }
      }
    }

    return false;
  }


  uint32_t AllocateThreadID() {
    while (true) {
      uint32_t Index = 0;
      uint32_t OriginalIDMask = HeartBeatEvents->NumClients.load();
      if (OriginalIDMask == ~0U) {
        // All slots taken
        return ~0U;
      }

      // Scan every index and try to find a free slot
      while (Index != 33) {
        uint32_t Slot = 1 << Index;
        if ((OriginalIDMask & Slot) == 0) {
          // Free slot, let's try and take it
          uint32_t NewIDMask = OriginalIDMask | Slot;

          if (HeartBeatEvents->NumClients.compare_exchange_strong(OriginalIDMask, NewIDMask)) {
            // We successfully got a slow
            return Index;
          }
          else {
            // We failed to get a slot, try again
            break;
          }
        }

        // Try next index
        ++Index;
      }
    }
  }

  /**
   * @brief Deallocates the thread ID from the clients
   *
   * @param LocalThreadID
   *
   * @return true is returned if we are the last thread to deallocate
   */
  bool DeallocateThreadID(uint32_t LocalThreadID) {
    uint32_t ThreadIDMask = (1 << LocalThreadID);
    while (true) {
      uint32_t OriginalIDMask = HeartBeatEvents->NumClients.load();
      uint32_t NewIDMask = OriginalIDMask & ~ThreadIDMask;
      if (HeartBeatEvents->NumClients.compare_exchange_strong(OriginalIDMask, NewIDMask)) {
        // We set the atomic. If the new mask is zero then we were the last to deallocate
        return NewIDMask == 0;
      }
      else {
        // Failed to deallocate, try again
      }
    }
  }

  struct HeartbeatClient {
    std::atomic<uint32_t> Type; // 1 = Server, 2 = Client
    std::atomic<uint32_t> HeartbeatCounter;
  };

  struct SHMHeartbeatEvent {
    std::atomic<uint32_t> Allocated;
    std::atomic<uint32_t> NumClients;
    HeartbeatClient Clients[0];
  };
  std::string EventName;
  std::unique_ptr<boost::interprocess::shared_memory_object> SHM;
  std::unique_ptr<boost::interprocess::mapped_region> SHMRegion;
  SHMHeartbeatEvent *HeartBeatEvents;

  bool Running{true};
  uint32_t ThreadID;
  std::thread HeartBeatThread;

  const std::chrono::milliseconds HeartBeatRate = std::chrono::seconds(1);
};

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  auto Args = FEX::ArgLoader::Get();

  FEX::Config::Value<uint8_t> CoreConfig{"Core", 0};
  FEX::Config::Value<bool> ConfigIPCClient{"IPCClient", false};
  FEX::Config::Value<bool> ConfigELFType{"ELFType", false};
  FEX::Config::Value<std::string> ConfigIPCID{"IPCID", "0"};

  char File[256]{};

  std::unique_ptr<IPCMessage> StateMessage;
  std::unique_ptr<IPCMessage> StateFile;
  std::unique_ptr<IPCEvent> HostEvent;
  std::unique_ptr<IPCEvent> ClientEvent;
  std::unique_ptr<IPCFlag> QuitFlag;

  // Heartbeat is the only thing robust enough to support client or server starting in any order
  printf("Initializing Heartbeat\n"); fflush(stdout);
  IPCHeartbeat HeartBeat(ConfigIPCID() + "IPCHeart_Lockstep", ConfigIPCClient());

  printf("Client? %s\n", ConfigIPCClient() ? "Yes" : "No"); fflush(stdout);

  if (ConfigIPCClient()) {
    if (!HeartBeat.WaitForServer()) {
      // Server managed to time out
      LogMan::Msg::E("[CLIENT %s] Timed out waiting for server", ConfigIPCID().c_str());
      return -1;
    }

    // Now that we know the server is online, create our objects
    StateMessage = std::make_unique<IPCMessage>(ConfigIPCID() + "IPCState_Lockstep", ConfigIPCClient(), sizeof(FEXCore::Core::CPUState), 1);
    StateFile = std::make_unique<IPCMessage>(ConfigIPCID() + "IPCFile_Lockstep", ConfigIPCClient(), 256, 1);
    HostEvent = std::make_unique<IPCEvent>(ConfigIPCID() + "IPCHost_Lockstep", ConfigIPCClient());
    ClientEvent = std::make_unique<IPCEvent>(ConfigIPCID() + "IPCClient_Lockstep", ConfigIPCClient());
    QuitFlag = std::make_unique<IPCFlag>(ConfigIPCID() + "IPCFlag_Lockstep", ConfigIPCClient());

    HeartBeat.EnableClient();
    HostEvent->Wait();
    StateFile->Recv(File, 256);
  }
  else {
    LogMan::Throw::A(!Args.empty(), "[SERVER %s] Not enough arguments", ConfigIPCID().c_str());
    strncpy(File, Args[0].c_str(), 256);

    // Create all of our objects now
    StateMessage = std::make_unique<IPCMessage>(ConfigIPCID() + "IPCState_Lockstep", ConfigIPCClient(), sizeof(FEXCore::Core::CPUState), 1);
    StateFile = std::make_unique<IPCMessage>(ConfigIPCID() + "IPCFile_Lockstep", ConfigIPCClient(), 256, 1);
    HostEvent = std::make_unique<IPCEvent>(ConfigIPCID() + "IPCHost_Lockstep", ConfigIPCClient());
    ClientEvent = std::make_unique<IPCEvent>(ConfigIPCID() + "IPCClient_Lockstep", ConfigIPCClient());
    QuitFlag = std::make_unique<IPCFlag>(ConfigIPCID() + "IPCFlag_Lockstep", ConfigIPCClient());

    HeartBeat.EnableServer();
    HeartBeat.WaitForClient();
    StateFile->Send(File, strlen(File));
    HostEvent->NotifyAll();
  }

  bool ShowProgress = false;
  uint64_t PCEnd;
  uint64_t PCStart = 1;
  auto LastTime = std::chrono::high_resolution_clock::now();

  FEXCore::Context::InitializeStaticTables();
  auto SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 36);
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig() > 3 ? FEXCore::Config::CONFIG_CUSTOM : CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, 1);
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, 1);

  if (CoreConfig() == 4) {
    FEXCore::Context::SetCustomCPUBackendFactory(CTX, VMFactory::CPUCreationFactory);
  }

  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);

  if (ConfigELFType()) {
    FEX::HarnessHelper::ELFCodeLoader Loader{File, {}, {},{}};
    bool Result = FEXCore::Context::InitCore(CTX, &Loader);
    printf("Did we Load? %s\n", Result ? "Yes" : "No");
  }
  else {
    FEX::HarnessHelper::HarnessCodeLoader Loader{File, nullptr};
    bool Result = FEXCore::Context::InitCore(CTX, &Loader);
    printf("Did we Load? %s\n", Result ? "Yes" : "No");
    ShowProgress = true;
    PCEnd = Loader.GetFinalRIP();
    PCStart = Loader.DefaultRIP();
  }

  FEXCore::Core::CPUState State1;
  bool MaskFlags = true;
  bool MaskGPRs = false;
  uint64_t LastRIP = 0;

  auto PrintProgress = [&](bool PrintFinal = false) {
    if (ShowProgress) {
      auto CurrentTime = std::chrono::high_resolution_clock::now();
      auto Diff = CurrentTime - LastTime;
      if (Diff >= std::chrono::seconds(1) || PrintFinal) {
        LastTime = CurrentTime;
        uint64_t CurrentRIP = State1.rip - PCStart;
        uint64_t EndRIP = PCEnd - PCStart;
        double Progress = static_cast<double>(CurrentRIP) / static_cast<double>(EndRIP) * 25.0;
        printf("Progress: [");
        for (uint32_t i = 0; i < 25; ++i) {
          printf("%c", (Progress > static_cast<double>(i)) ? '#' : '|');
        }
        printf("] RIP: 0x%lx\n", State1.rip);
      }
    }
  };

  uint32_t ErrorLocation = 0;
  bool Done = false;
  bool ServerTimedOut = false;
  while (!Done)
  {
    if (MaskFlags) {
      // We need to reset the CPU flags to somethign standard so we don't need to handle flags in this case
      FEXCore::Core::CPUState CPUState;
      FEXCore::Context::GetCPUState(CTX, &CPUState);
      for (int i = 0; i < 32; ++i) {
        CPUState.flags[i] = 0;
      }
      CPUState.flags[1] = 1; // Default state
      FEXCore::Context::SetCPUState(CTX, &CPUState);
    }

    FEXCore::Context::ExitReason ExitReason;
    ExitReason = FEXCore::Context::RunUntilExit(CTX);
    FEXCore::Context::GetCPUState(CTX, &State1);

    if (ExitReason == FEXCore::Context::ExitReason::EXIT_SHUTDOWN) {
      Done = true;
    }
    else if (ExitReason == FEXCore::Context::ExitReason::EXIT_UNKNOWNERROR) {
      QuitFlag->GetFlag()->Set();
      ErrorLocation = -2;
      break;
    }

    if (!ConfigIPCClient()) {
      FEXCore::Core::CPUState State2;
      if (StateMessage->Recv(&State2, sizeof(FEXCore::Core::CPUState)) == 0) {
        // We had a time out
        // This can happen when the client managed to early exit
        LogMan::Msg::E("Client Timed out");
        ErrorLocation = -2;
        QuitFlag->GetFlag()->Set();
        ClientEvent->NotifyAll();
        break;
      }

      PrintProgress();

      uint64_t MatchMask = (1ULL << 36) - 1;
      if (MaskFlags) {
        MatchMask &= ~(1ULL << 35); // Remove FLAGS for now
      }

      if (MaskGPRs) {
        MatchMask &= ~((1ULL << 35) - 1);
      }

      bool Matches = FEX::HarnessHelper::CompareStates(State1, State2, MatchMask, true);

      if (!Matches) {
        LogMan::Msg::E("[SERVER %s] Stated ended up different at RIPS 0x%lx - 0x%lx - LastRIP: 0x%lx\n", ConfigIPCID().c_str(), State1.rip, State2.rip, LastRIP);
        ErrorLocation = -3;
        QuitFlag->GetFlag()->Set();
        break;
      }
      LastRIP = State1.rip;
      ClientEvent->NotifyAll();
    }
    else {
      StateMessage->Send(&State1, sizeof(FEXCore::Core::CPUState));
      if (!ClientEvent->WaitFor(boost::get_system_time() + boost::posix_time::seconds(10))) {
        // Timed out
        LogMan::Msg::E("Server timed out");
        QuitFlag->GetFlag()->Set();
        ErrorLocation = -1;
        ServerTimedOut = true;
      }
    }

    if (QuitFlag->GetFlag()->Load()) {
      break;
    }
  }

  // Some cleanup that needs to be done in case some side failed
  if (!ConfigIPCClient()) {
    ClientEvent->NotifyAll();
  }
  else {
    if (!ServerTimedOut) {
      // Send the latest state to the server so it can die
      StateMessage->Send(&State1, sizeof(FEXCore::Core::CPUState));
    }
  }

  FEXCore::Context::GetCPUState(CTX, &State1);
  PrintProgress(true);

  if (ErrorLocation == 0) {
    // One final check to make sure the RIP ended up in the correct location
    if (State1.rip != PCEnd) {
      ErrorLocation = State1.rip;
    }
  }

  if (ErrorLocation != 0) {
    LogMan::Msg::E("Error Location: 0x%lx", ErrorLocation);
  }

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::SHM::DestroyRegion(SHM);
  FEX::Config::Shutdown();
  return ErrorLocation != 0;
}

#include <boost/interprocess/detail/config_end.hpp>
