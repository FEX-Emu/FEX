#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <vector>

namespace SocketUtil {
  size_t ReadDataFromSocket(int Socket, std::vector<uint8_t> *Data) {
    size_t CurrentRead{};
    while (true) {
      struct iovec iov {
        .iov_base = &Data->at(CurrentRead),
        .iov_len = Data->size() - CurrentRead,
      };

      struct msghdr msg {
        .msg_name = nullptr,
        .msg_namelen = 0,
        .msg_iov = &iov,
        .msg_iovlen = 1,
      };

      ssize_t Read = recvmsg(Socket, &msg, 0);
      if (Read <= msg.msg_iov->iov_len) {
        CurrentRead += Read;
        if (CurrentRead == Data->size()) {
          Data->resize(Data->size() << 1);
        }
        else {
          // No more to read
          break;
        }
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

    return CurrentRead;
  }
}
