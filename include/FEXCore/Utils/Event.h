#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>

class Event final {
private:
/**
 * @brief Literally just an atomic bool that we are using for this class
 */
class Flag final {
public:
  bool TestAndSet(bool SetValue = true) {
    bool Expected = !SetValue;
    return Value.compare_exchange_strong(Expected, SetValue);
  }

  bool TestAndClear() {
    return TestAndSet(false);
  }

private:
  std::atomic_bool Value {false};
};

public:
  ~Event() {
    NotifyAll();
  }
  void NotifyOne() {
    if (FlagObject.TestAndSet()) {
      std::lock_guard<std::mutex> lk(MutexObject);
      CondObject.notify_one();
    }
  }

  void NotifyAll() {
    if (FlagObject.TestAndSet()) {
      std::lock_guard<std::mutex> lk(MutexObject);
      CondObject.notify_all();
    }
  }

  void Wait() {
    // Have we signaled before we started waiting?
    if (FlagObject.TestAndClear())
      return;

    std::unique_lock<std::mutex> lk(MutexObject);
    CondObject.wait(lk, [this]{ return FlagObject.TestAndClear(); });
  }

  template<class Rep, class Period>
  bool WaitFor(std::chrono::duration<Rep, Period> const& time) {
    // Have we signaled before we started waiting?
    if (FlagObject.TestAndClear())
      return true;

    std::unique_lock<std::mutex> lk(MutexObject);
    bool DidSignal = CondObject.wait_for(lk, time, [this]{ return FlagObject.TestAndClear(); });
    return DidSignal;
  }

private:
  Flag FlagObject;
  std::mutex MutexObject;
  std::condition_variable CondObject;
};
