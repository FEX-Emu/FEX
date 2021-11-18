#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unistd.h>

struct CrossArchEvent final {
  std::atomic<uint32_t> Futex;
};

static void WaitForWorkFunc(CrossArchEvent *Event) {

  // Wait for Futex value to become 1
  while (true) {

    // First step compare it with 1 already and see if we can early out
    uint32_t One = 1;
    if (Event->Futex.compare_exchange_strong(One, 0)) {
      return;
    }

    int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
    int Res = syscall(SYS_futex,
        &Event->Futex,
        Op,
        nullptr, // Timeout
        nullptr, // Addr
        0);
  }
}

static void NotifyWorkFunc(CrossArchEvent *Event) {
  uint32_t Zero = 0;
  if (Event->Futex.compare_exchange_strong(Zero, 1)) {
    int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
    syscall(SYS_futex,
      &Event->Futex,
      Op,
      nullptr, // Timeout
      nullptr, // Addr
      0);
  }
}

//class CrossArchEvent final {
//  private:
//  /**
//   * @brief Literally just an atomic bool that we are using for this class
//   */
//  class Flag final {
//  public:
//    bool TestAndSet(bool SetValue = true) {
//      bool Expected = !SetValue;
//      return Value.compare_exchange_strong(Expected, SetValue);
//    }
//
//    bool TestAndClear() {
//      return TestAndSet(false);
//    }
//
//    bool Get() {
//      return Value.load();
//    }
//
//  private:
//    std::atomic_bool Value {false};
//  };
//
//  public:
//    ~CrossArchEvent() {
//      NotifyAll();
//    }
//    void NotifyOne() {
//      if (FlagObject.TestAndSet()) {
//        std::lock_guard<std::mutex> lk(MutexObject);
//        CondObject.notify_one();
//      }
//    }
//
//    void NotifyAll() {
//      if (FlagObject.TestAndSet()) {
//        std::lock_guard<std::mutex> lk(MutexObject);
//        CondObject.notify_all();
//      }
//    }
//
//    void Wait() {
//      // Have we signaled before we started waiting?
//      if (FlagObject.TestAndClear())
//        return;
//
//      std::unique_lock<std::mutex> lk(MutexObject);
//      CondObject.wait(lk, [this]{ return FlagObject.TestAndClear(); });
//    }
//
//    template<class Rep, class Period>
//    bool WaitFor(std::chrono::duration<Rep, Period> const& time) {
//      // Have we signaled before we started waiting?
//      if (FlagObject.TestAndClear())
//        return true;
//
//      std::unique_lock<std::mutex> lk(MutexObject);
//      bool DidSignal = CondObject.wait_for(lk, time, [this]{ return FlagObject.TestAndClear(); });
//      return DidSignal;
//    }
//    void BusyWaitForWork() {
//      while (!FlagObject.Get());
//    }
//
//  private:
//    std::mutex MutexObject;
//    std::condition_variable CondObject;
//    Flag FlagObject;
//};


