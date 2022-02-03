#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>

/**
 * @brief Literally just an atomic bool that we are using for this class
 */
class Flag final {
protected:
  friend class Event;
  friend class LatchEvent;

  bool TestAndSet(bool SetValue = true) {
    bool Expected = !SetValue;
    return Value.compare_exchange_strong(Expected, SetValue);
  }

  bool TestAndClear() {
    return TestAndSet(false);
  }

  bool Test() const {
    return Value.load();
  }

  void Set() {
    Value = true;
  }

private:
  std::atomic_bool Value {false};
};


class Event final {
private:
public:
  ~Event() {
    NotifyAll();
  }

  bool Test() const {
    return FlagObject.Test();
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

  void Lock() {
    do {
      FlagObject.TestAndSet();
      CondObject.notify_all();
    } while (!MutexObject.try_lock());
  }

  void Unlock() {
    MutexObject.unlock();
    FlagObject.TestAndSet();
    CondObject.notify_all();
  }

private:
  Flag FlagObject;
  std::mutex MutexObject;
  std::condition_variable CondObject;
};

/**
 * @brief Once this event is latched then it never resets
 */
class LatchEvent final {
public:

  bool Test() const {
    return FlagObject.Test();
  }

  void NotifyAll() {
    if (!FlagObject.Test()) {
      FlagObject.Set();
      CondObject.notify_all();
    }
  }

  void Wait() {
    if (FlagObject.Test()) return;

    std::unique_lock<std::mutex> lk(MutexObject);
    CondObject.wait(lk, [this]{ return FlagObject.Test(); });
  }

private:
  Flag FlagObject;
  std::mutex MutexObject;
  std::condition_variable CondObject;
};
