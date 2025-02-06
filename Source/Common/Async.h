// SPDX-License-Identifier: MIT
/**
 * Helper framework to enable asynchronous IO operations on file descriptor objects (networking, files).
 *
 * Strongly inspired by Boost.Asio.
 */
#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <optional>
#include <poll.h>
#include <span>
#include <utility>
#include <vector>

#include <FEXCore/fextl/functional.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/vector.h>

namespace fasio {

enum class error {
  success,
  timeout,      // User-specified timeout expired
  eof,          // Permanently reached end of data stream (e.g. because socket connection was closed by peer)
  invalid,      // Invalid input parameters
  generic_errno // Read errno for details
};

/**
 * This selects which action to trigger when returning from a reactor callback.
 * The default (drop) will drop the callback so that the caller can register
 * a new one.
 */
enum class post_callback {
  drop,         // Drop the callback
  repeat,       // Continue using the same callback
  stop_reactor, // Triggers exit from run()
};

/**
 * Core event loop for asynchronous code. Corresponds to asio::io_context,
 * specialized for multiplexing file descriptors via ppoll().
 *
 * A reactor tracks a set of file descriptors and calls user-provided callbacks
 * when they become ready. For example, the callback for a network socket will
 * be called when data is ready to be reveived on the socket.
 *
 */
struct poll_reactor {
private:
  std::vector<pollfd> PollFDs;
  std::optional<int> CurrentFD; // FD that is currently being processed

  int AsyncStopRequest[2] = {-1, -1};

  // Maps FD to callback
  fextl::map<int, fextl::move_only_function<post_callback(error)>> callbacks;

  struct Event {
    pollfd FD;
    bool Erase = false;
    bool Insert = false;
  };
  std::vector<Event> QueuedEvents;

public:
  ~poll_reactor() {
    if (AsyncStopRequest[0]) {
      ::close(AsyncStopRequest[0]);
      ::close(AsyncStopRequest[1]);
    }
  }

  // Adds an internal FD to wake up and exit the reactor when stop_async() is called from any thread.
  void enable_async_stop() {
    ::pipe(AsyncStopRequest);
    PollFDs.push_back(pollfd {.fd = AsyncStopRequest[0], .events = POLLHUP, .revents = 0});
    callbacks[AsyncStopRequest[0]] = [](error) {
      return post_callback::stop_reactor;
    };
  }

  void stop_async() {
    if (AsyncStopRequest[1] == -1) {
      ERROR_AND_DIE_FMT("Tried to use stop_async without calling enable_async_stop during setup");
    }
    // Wake up run() thread by closing this pipe endpoint
    close(AsyncStopRequest[1]);
  }

  error run(std::optional<std::chrono::nanoseconds> Timeout = std::nullopt) {
    // Process events queued before entering wait loop
    update_fd_list();

    timespec ts = to_timespec(Timeout.value_or(std::chrono::nanoseconds {0}));

    while (true) {
      int Result = ppoll(PollFDs.data(), PollFDs.size(), Timeout ? &ts : nullptr, nullptr);

      if (Result < 0) {
        callbacks.clear();
        return error::generic_errno;
      } else if (Result == 0) {
        callbacks.clear();
        return error::timeout;
      } else {
        bool exit_requested = false;

        // Walk the FDs and see if we got any results
        for (auto& ActiveFD : PollFDs) {
          if (ActiveFD.revents == 0) {
            continue;
          }
          if (Result-- == 0) {
            break;
          }

          if (ActiveFD.revents & POLLIN) {
            // NOTE: For sockets, this is triggered on close, too. Pipes only report POLLHUP, however.
            CurrentFD = ActiveFD.fd;

            auto Callback = std::move(callbacks[ActiveFD.fd]);
            if (!Callback) {
              ERROR_AND_DIE_FMT("Data available for reading on FD {} but no read callback registered", ActiveFD.fd);
            }
            auto Ret = Callback(error::success);
            if (Ret == post_callback::repeat) {
              callbacks[ActiveFD.fd] = std::move(Callback);
            } else if (Ret == post_callback::drop) {
              // If no new callback was registered, drop the FD from the list and skip any remaining events
              if (!callbacks.contains(ActiveFD.fd)) {
                QueuedEvents.push_back(Event {.FD = {.fd = ActiveFD.fd}, .Erase = true});
                ActiveFD.revents = 0;
              }
            } else if (Ret == post_callback::stop_reactor) {
              exit_requested = true;
            }
            CurrentFD.reset();
          }
          if (ActiveFD.revents & (POLLHUP | POLLERR | POLLNVAL | POLLRDHUP)) {
            auto Callback = std::move(callbacks[ActiveFD.fd]);
            if (Callback) {
              exit_requested |= (Callback(error::eof) == post_callback::stop_reactor);
            }
            // Error or hangup, erase the socket from our list
            QueuedEvents.push_back(Event {.FD = {.fd = ActiveFD.fd}, .Erase = true});
          }

          ActiveFD.revents = 0;
        }

        if (exit_requested) {
          callbacks.clear();
          return error::success;
        }

        update_fd_list();
      }
    }
  }

  void bind_handler(pollfd FD, fextl::move_only_function<post_callback(error)> Callback) {
    [[maybe_unused]] auto Previous = std::exchange(callbacks[FD.fd], std::move(Callback));
    assert(!Previous && "May not queue multiple async operations");

    // Add the FD to the poll list if it's not already contained
    if (CurrentFD != FD.fd && PollFDs.end() == std::find_if(PollFDs.begin(), PollFDs.end(), [&](auto& Prev) { return FD.fd == Prev.fd; })) {
      QueuedEvents.push_back(Event {.FD = FD, .Insert = true});
    }
  }

private:
  timespec to_timespec(std::chrono::nanoseconds Duration) {
    timespec Timespec {};
    auto Seconds = std::chrono::duration_cast<std::chrono::seconds>(Duration);
    Timespec.tv_sec = Seconds.count();
    Timespec.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(Duration - Seconds).count();
    return Timespec;
  }

  void update_fd_list() {
    for (auto& Event : QueuedEvents) {
      if (Event.Erase) {
        std::iter_swap(std::find_if(PollFDs.begin(), PollFDs.end(), [&](auto& FD) { return FD.fd == Event.FD.fd; }), std::prev(PollFDs.end()));
        PollFDs.pop_back();
        callbacks.erase(Event.FD.fd);
      }

      if (Event.Insert) {
        PollFDs.push_back(Event.FD);
      }
    }
    QueuedEvents.clear();
  }
};

/**
 * Corresponds to asio::mutable_buffer.
 */
struct mutable_buffer {
  std::span<std::byte> Data;
  mutable_buffer* Next = nullptr;

  // Optional FD to send/receive via ancillary buffer.
  // This may only be used with non-empty data, and there may only be up to one FD per buffer chain
  std::optional<int*> FD;

  size_t size() const {
    size_t Ret = 0;
    const mutable_buffer* Current = this;
    do {
      Ret += Current->Data.size_bytes();
      Current = Current->Next;
    } while (Current);

    if (Ret == 0) {
      assert(!FD);
    }
    return Ret;
  }

  int consume_fd() {
    assert(FD);
    return **std::exchange(FD, std::nullopt);
  }

  mutable_buffer& operator+=(size_t NumBytes) {
    mutable_buffer* Current = this;
    while (Current->Next && NumBytes >= Current->Data.size_bytes()) {
      NumBytes -= Data.size_bytes();
      Current = Current->Next;
      assert(Current->FD == std::nullopt);
    }
    auto FD = std::exchange(this->FD, std::nullopt);
    *this = *Current;
    Data = Data.subspan(std::min(Data.size_bytes(), NumBytes));
    this->FD = FD;
    return *Current;
  }

  size_t count_chunks() const {
    size_t Ret = 1;
    const mutable_buffer* Current = this;
    while (Current->Next) {
      Current = Current->Next;
      ++Ret;
    }
    return Ret;
  }
};

inline mutable_buffer Chained(std::span<mutable_buffer> Buffers) {
  for (size_t i = 0; i + 1 < Buffers.size(); ++i) {
    Buffers[i].Next = &Buffers[i + 1];
  }
  return Buffers[0];
}

/**
 * Corresponds to asio::dynamic_vector_buffer.
 */
struct dynamic_vector_buffer {
  fextl::vector<std::byte>& Data;

  // Maximum number of bytes to grow to
  size_t max_size = Data.capacity();
};

/**
 * Asynchronously reads data from the given stream until MatchPredicate reports a match. The read
 * is queued to the stream's reactor and will progress whenever data is available.
 *
 * MatchPredicate must have the signature pair<Iter, bool>(Iter, Iter):
 * - The input iterators provide the range of new data bytes
 * - The returned boolean indicates if a match was found
 * - The returned iterator is the match location or the location at which to continue testing after the next read
 *
 * The read data will be appended to Buffers. Data past the match returned from the last read data will also be included.
 *
 * Corresponds to asio::async_read_until.
 */
template<typename AsyncReadStream, typename MatchPredicate, typename OnComplete>
requires std::is_invocable_r_v<void, OnComplete, error, size_t>
void async_read_until(AsyncReadStream& Stream, dynamic_vector_buffer Buffers, MatchPredicate Predicate, OnComplete UserCallback) {
  struct Callback {
    size_t BeginPos;
    size_t EndPos;
    AsyncReadStream& Stream;
    dynamic_vector_buffer Buffers;
    MatchPredicate Predicate;
    OnComplete UserCallback;

    void operator()(error Err, size_t BytesRead, std::optional<int> FD) {
      if (Err != error::success) {
        UserCallback(Err, 0);
        return;
      }

      // Start with the predicate check to avoid fetching data unnecessarily
      EndPos += BytesRead;
      if (EndPos != BeginPos) {
        auto Begin = Buffers.Data.begin() + BeginPos;
        auto End = Buffers.Data.begin() + EndPos;
        auto [It, Found] = Predicate(Begin, End);
        BeginPos = It - Buffers.Data.begin();
        if (Found) {
          Buffers.Data.resize(EndPos); // Shrink down to size of data actually received
          UserCallback(error::success, BeginPos);
          return;
        }
      }

      // Fill the entire remaining capacity, or resize for a minimum of 512 bytes
      auto BytesToRead = std::max<size_t>(std::min(Buffers.Data.capacity(), Buffers.max_size) - EndPos, 512);
      if (Buffers.Data.size() + BytesToRead > Buffers.max_size) {
        ERROR_AND_DIE_FMT("Out of buffer space");
      }

      Buffers.Data.resize(EndPos + BytesToRead);

      // Queue data read.
      // On completion, Reader will check if enough data was received and will queue more reads if needed.
      Stream.async_read_some(mutable_buffer {std::span {Buffers.Data}}, *this);
    }
  };

  // Check existing data for a predicate match, then initiate async reading if necessary
  Callback {0, Buffers.Data.size(), Stream, Buffers, std::move(Predicate), std::move(UserCallback)}(error::success, 0, std::nullopt);
}

using read_callback = fextl::move_only_function<void(error, size_t, std::optional<int>)>;

/**
 * Synchronously writes fixed-length data to the given Stream.
 *
 * The length is inferred from the size of the input buffer(s).
 *
 * Corresponds to asio::write.
 */
template<typename AsyncReadStream>
std::size_t write(AsyncReadStream& Stream, mutable_buffer Buffers, error& ec) {
  size_t TotalBytesWritten = 0;
  while (Buffers.size() != 0 || Buffers.FD) {
    auto BytesWritten = Stream.write_some(Buffers, ec);
    TotalBytesWritten += BytesWritten;
    if (Buffers.FD) {
      (void)Buffers.consume_fd();
    }
    Buffers += BytesWritten;
    if (ec != error::success) {
      return TotalBytesWritten;
    }
  }
  ec = error::success;
  return TotalBytesWritten;
}

/**
 * Owning RAII wrapper around a file descriptor.
 *
 * Corresponds to asio::posix::descriptor.
 */
struct posix_descriptor {
  poll_reactor* Reactor = nullptr;
  int FD = -1;

  posix_descriptor(poll_reactor& Reactor, int FD)
    : Reactor(&Reactor)
    , FD(FD) {}

  posix_descriptor(posix_descriptor&& Other)
    : Reactor(Other.Reactor)
    , FD(std::exchange(Other.FD, -1)) {}

  posix_descriptor& operator=(posix_descriptor&& Other) {
    posix_descriptor::~posix_descriptor();
    Reactor = Other.Reactor;
    FD = std::exchange(Other.FD, -1);
    return *this;
  }

  ~posix_descriptor() {
    if (FD != -1) {
      close(FD);
    }
  }

  /**
   * Wait until there is data available to read on this object, then execute the given callback
   */
  template<typename Fn>
  requires std::is_invocable_r_v<post_callback, Fn, error>
  void async_wait(Fn Callback) {
    Reactor->bind_handler(
      pollfd {
        .fd = FD,
        .events = POLLIN,
        .revents = 0,
      },
      std::move(Callback));
  }
};

} // namespace fasio
