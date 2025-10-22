// SPDX-License-Identifier: MIT
/**
 * Socket wrappers for asynchronous programming with fasio
 */
#pragma once

#include <Common/Async.h>

#include <sys/socket.h>
#include <sys/un.h>

namespace fasio {

/**
 * Non-owning wrapper around a socket.
 *
 * Corresponds to asio::local::stream_protocol::socket.
 */
struct tcp_socket {
  poll_reactor* Reactor = nullptr;
  int FD;

  // Constructor for synchronous and asynchronous operation
  tcp_socket(poll_reactor& Reactor_, int FD_)
    : Reactor(&Reactor_)
    , FD(FD_) {}

  // Constructor for purely synchronous operation
  tcp_socket(int FD_)
    : FD(FD_) {}

  /**
   * Queues an asynchronous operation that will run the completion callback
   * once at least one byte of data was received
   */
  template<typename OnComplete>
  requires std::is_invocable_r_v<void, OnComplete, error, size_t, std::optional<int>>
  void async_read_some(mutable_buffer Buffers, OnComplete UserCallback) {
    auto Callback = [Buffers, Socket = FD, UserCallback = std::move(UserCallback)](error ec) mutable {
      if (ec != error::success) {
        UserCallback(ec, 0, std::nullopt);
        return post_callback::drop;
      }

      auto BytesRead = read_some_from_fd(Buffers, ec, Socket);
      if (ec != error::success) {
        UserCallback(ec, BytesRead, std::nullopt);
      } else {
        UserCallback(ec, BytesRead, Buffers.FD ? std::optional {**Buffers.FD} : std::nullopt);
      }
      return post_callback::drop;
    };

    Reactor->bind_handler(
      pollfd {
        .fd = FD,
        .events = POLLIN | POLLPRI | POLLRDHUP,
        .revents = 0,
      },
      std::move(Callback));
  }

  /**
   * Blocks until at least one byte of data was received
   */
  size_t read_some(const mutable_buffer& Buffers, error& ec) {
    return read_some_from_fd(Buffers, ec, FD);
  }

  /**
   * Blocks until at least one byte of data was sent
   */
  size_t write_some(const mutable_buffer& Buffers, error& ec) {
    auto iov = (iovec*)alloca(sizeof(mutable_buffer) * Buffers.count_chunks());
    size_t NumIovs = 0;
    for (auto Buffer = &Buffers; Buffer; Buffer = Buffer->Next) {
      iov[NumIovs].iov_base = Buffer->Data.data();
      iov[NumIovs].iov_len = Buffer->Data.size_bytes();
      ++NumIovs;
    }
    msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = iov,
      .msg_iovlen = NumIovs,
    };

    // Setup the ancillary buffer. This is where we will be getting pipe FDs
    // We only need 4 bytes for the FD
    constexpr size_t CMSG_SIZE = CMSG_SPACE(sizeof(int));
    alignas(cmsghdr) uint8_t AncBuf[CMSG_SIZE];

    if (Buffers.FD) {
      // Enable ancillary buffer
      msg.msg_control = AncBuf;
      msg.msg_controllen = CMSG_SIZE;

      // Now we need to setup the ancillary buffer data. We are only sending an FD
      cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
      cmsg->cmsg_len = CMSG_LEN(sizeof(int));
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;

      // We are giving the daemon the write side of the pipe
      memcpy(CMSG_DATA(cmsg), Buffers.FD.value(), sizeof(int));
    }

    ssize_t Ret;
    do {
      Ret = ::sendmsg(FD, &msg, 0);
    } while (Ret < 0 && (errno == EINTR || errno == EAGAIN));
    if (Ret < 0) {
      ec = error::generic_errno;
      return 0;
    }
    ec = error::success;
    return Ret;
  }

private:
  static size_t read_some_from_fd(const mutable_buffer& Buffers, error& ec, int FD) {
    auto iov = (iovec*)alloca(sizeof(mutable_buffer) * Buffers.count_chunks());
    size_t NumIovs = 0;
    for (auto Buffer = &Buffers; Buffer; Buffer = Buffer->Next) {
      iov[NumIovs].iov_base = Buffer->Data.data();
      iov[NumIovs].iov_len = Buffer->Data.size_bytes();
      ++NumIovs;
    }
    msghdr msg {
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = iov,
      .msg_iovlen = NumIovs,
    };

    // If requested, set up a 4-byte ancillary buffer for receiving a file descriptor
    constexpr size_t CMSG_SIZE = CMSG_SPACE(sizeof(int));
    alignas(cmsghdr) uint8_t AncBuf[CMSG_SIZE];

    if (Buffers.FD) {
      // Enable ancillary buffer
      msg.msg_control = AncBuf;
      msg.msg_controllen = CMSG_SIZE;
    }

    ssize_t BytesRead;
    do {
      BytesRead = ::recvmsg(FD, &msg, 0);
    } while (BytesRead < 0 && (errno == EINTR || errno == EAGAIN));
    if (BytesRead < 0) {
      if (errno != 0) {
        ec = error::generic_errno;
        return 0;
      }
    } else if (BytesRead == 0) {
      ec = error::eof;
      return 0;
    }

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (Buffers.FD &&
        (cmsg == nullptr || cmsg->cmsg_len != CMSG_LEN(sizeof(int)) || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)) {
      // Not a failure since some data was read for the main message
      **Buffers.FD = -1;
      ec = error::success;
      return BytesRead;
    }

    if (Buffers.FD) {
      memcpy(*Buffers.FD, CMSG_DATA(cmsg), sizeof(FD));
    }

    ec = error::success;
    return BytesRead;
  }
};

/**
 * Owning wrapper around a server socket that listens for connections after
 * creation. Clients can be accepted asynchronously using async_accept().
 *
 * Corresponds to asio::local::stream_protocol::acceptor.
 */
struct tcp_acceptor {
  poll_reactor& Reactor;
  int FD;

  tcp_acceptor(tcp_acceptor&& other)
    : Reactor(other.Reactor)
    , FD(other.FD) {
    other.FD = -1;
  }

  ~tcp_acceptor() {
    if (FD != -1) {
      ::close(FD);
    }
  }

  tcp_acceptor& operator=(tcp_acceptor&& other) {
    FD = std::exchange(other.FD, -1);
    return *this;
  }

  static std::optional<tcp_acceptor> create(poll_reactor& Reactor, bool abstract, std::string_view Name, int MaxPending = SOMAXCONN) {
    // Create the initial unix socket
    int FD = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (FD == -1) {
      return {};
    }

    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;

    if (Name.size() > sizeof(addr.sun_path) - 1) {
      ERROR_AND_DIE_FMT("Invalid FEXServer socket name: {}", Name);
    }

    auto NameEnd = addr.sun_path;
    if (!abstract) {
      // sun_path is null-terminated
      NameEnd = std::copy(Name.begin(), Name.end(), addr.sun_path);
      *NameEnd++ = 0;
    } else {
      // Abstract AF_UNIX sockets start with \0 but aren't null-terminated
      addr.sun_path[0] = 0;
      NameEnd = std::copy(Name.begin(), Name.end(), addr.sun_path + 1);
    }

    // Bind the socket to the path
    int Result = bind(FD, reinterpret_cast<sockaddr*>(&addr), sizeof(addr.sun_family) + (NameEnd - addr.sun_path));
    if (Result == -1) {
      ::close(FD);
      return {};
    }

    Result = ::listen(FD, MaxPending);
    if (Result == -1) {
      ::close(FD);
      return {};
    }

    return tcp_acceptor(Reactor, FD);
  }

  void async_accept(fextl::move_only_function<post_callback(error, std::optional<tcp_socket>)> OnAccept) {
    Reactor.bind_handler(
      {
        .fd = FD,
        .events = POLLIN,
        .revents = 0,
      },
      [ServerFD = FD, &Reactor = Reactor, OnAccept = std::move(OnAccept)](error ec) mutable {
        if (ec != error::success) {
          return post_callback::drop;
        }

        sockaddr_storage Addr {};
        socklen_t AddrSize {};
        int NewFD;
        do {
          NewFD = ::accept(ServerFD, reinterpret_cast<sockaddr*>(&Addr), &AddrSize);
        } while (NewFD < 0 && (errno == EINTR || errno == EAGAIN));
        if (NewFD < 0) {
          return OnAccept(error::generic_errno, std::nullopt);
        }

        return OnAccept(error::success, tcp_socket {Reactor, NewFD});
      });
  }

private:
  tcp_acceptor(poll_reactor& Reactor_, int FD_)
    : Reactor(Reactor_)
    , FD(FD_) {}
};
static_assert(!std::is_copy_constructible_v<tcp_acceptor>);
static_assert(!std::is_copy_assignable_v<tcp_acceptor>);

} // namespace fasio
