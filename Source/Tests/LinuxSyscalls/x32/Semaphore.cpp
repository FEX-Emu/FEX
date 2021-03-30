/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

namespace FEX::HLE::x32 {
  // Define the IPC ops
  enum IPCOp {
    OP_SEMOP      = 1,
    OP_SEMGET     = 2,
    OP_SEMCTL     = 3,
    OP_SEMTIMEDOP = 4,
    OP_MSGSND     = 11,
    OP_MSGRCV     = 12,
    OP_MSGGET     = 13,
    OP_MSGCTL     = 14,
    OP_SHMAT      = 21,
    OP_SHMDT      = 22,
    OP_SHMGET     = 23,
    OP_SHMCTL     = 24,
  };

  struct msgbuf_32 {
    uint32_t mtype;
    char mtext[1];
  };

  struct ipc_perm_32 {
    uint32_t key;
    uint16_t uid;
    uint16_t gid;
    uint16_t cuid;
    uint16_t cgid;
    uint16_t mode;
    uint16_t seq;

    ipc_perm_32() = delete;

    operator struct ipc_perm() const {
      struct ipc_perm perm;
      perm.__key = key;
      perm.uid   = uid;
      perm.gid   = gid;
      perm.cuid  = cuid;
      perm.cgid  = cgid;
      perm.mode  = mode;
      perm.__seq = seq;
      return perm;
    }

    ipc_perm_32(struct ipc_perm perm) {
      key  = perm.__key;
      uid  = perm.uid;
      gid  = perm.gid;
      cuid = perm.cuid;
      cgid = perm.cgid;
      mode = perm.mode;
      seq  = perm.__seq;
    }
  };

  static_assert(std::is_trivial<ipc_perm_32>::value, "Needs to be trivial");
  static_assert(sizeof(ipc_perm_32) == 16, "Incorrect size");

  struct ipc_perm_64 {
    uint32_t key;
    uint32_t uid;
    uint32_t gid;
    uint32_t cuid;
    uint32_t cgid;
    uint16_t mode;
    uint16_t _pad1;
    uint16_t seq;
    uint16_t _pad2;
    compat_ulong_t _pad[2];

    ipc_perm_64() = delete;

    operator struct ipc_perm() const {
      struct ipc_perm perm;
      perm.__key = key;
      perm.uid   = uid;
      perm.gid   = gid;
      perm.cuid  = cuid;
      perm.cgid  = cgid;
      perm.mode  = mode;
      perm.__seq = seq;
      return perm;
    }

    ipc_perm_64(struct ipc_perm perm) {
      key  = perm.__key;
      uid  = perm.uid;
      gid  = perm.gid;
      cuid = perm.cuid;
      cgid = perm.cgid;
      mode = perm.mode;
      seq  = perm.__seq;
    }
  };

  static_assert(std::is_trivial<ipc_perm_64>::value, "Needs to be trivial");
  static_assert(sizeof(ipc_perm_64) == 36, "Incorrect size");

  struct shmid_ds_32 {
    ipc_perm_32 shm_perm;
    int32_t shm_segsz;
    int32_t shm_atime;
    int32_t shm_dtime;
    int32_t shm_ctime;
    uint16_t shm_cpid;
    uint16_t shm_lpid;
    uint16_t shm_nattch;
    uint16_t shm_unused;
    uint32_t shm_unused2;
    uint32_t shm_unused3;

    shmid_ds_32() = delete;

    operator struct shmid_ds() const {
      struct shmid_ds buf;
      buf.shm_perm = shm_perm;

      buf.shm_segsz = shm_segsz;
      buf.shm_atime = shm_atime;
      buf.shm_dtime = shm_dtime;
      buf.shm_ctime = shm_ctime;
      buf.shm_cpid = shm_cpid;
      buf.shm_lpid = shm_lpid;
      buf.shm_nattch = shm_nattch;
      return buf;
    }

    shmid_ds_32(struct shmid_ds buf)
      : shm_perm {buf.shm_perm} {
      shm_segsz = buf.shm_segsz;
      shm_atime = buf.shm_atime;
      shm_dtime = buf.shm_dtime;
      shm_ctime = buf.shm_ctime;
      shm_cpid = buf.shm_cpid;
      shm_lpid = buf.shm_lpid;
      shm_nattch = buf.shm_nattch;
    }
  };

  static_assert(std::is_trivial<shmid_ds_32>::value, "Needs to be trivial");
  static_assert(sizeof(shmid_ds_32) == 48, "Incorrect size");

  struct shmid_ds_64 {
    ipc_perm_64 shm_perm;
    compat_size_t shm_segsz;
    compat_ulong_t shm_atime;
    compat_ulong_t shm_atime_high;
    compat_ulong_t shm_dtime;
    compat_ulong_t shm_dtime_high;
    compat_ulong_t shm_ctime;
    compat_ulong_t shm_ctime_high;
    int32_t shm_cpid;
    int32_t shm_lpid;
    compat_ulong_t shm_nattch;
    compat_ulong_t shm_unused4;
    compat_ulong_t shm_unused5;

    shmid_ds_64() = delete;

    operator struct shmid_ds() const {
      struct shmid_ds buf;
      buf.shm_perm = shm_perm;

      buf.shm_segsz = shm_segsz;
      buf.shm_atime = shm_atime_high;
      buf.shm_atime <<= 32;
      buf.shm_atime |= shm_atime;

      buf.shm_dtime = shm_dtime_high;
      buf.shm_dtime <<= 32;
      buf.shm_dtime |= shm_dtime;

      buf.shm_ctime = shm_ctime_high;
      buf.shm_ctime <<= 32;
      buf.shm_ctime |= shm_ctime;

      buf.shm_cpid = shm_cpid;
      buf.shm_lpid = shm_lpid;
      buf.shm_nattch = shm_nattch;
      return buf;
    }

    shmid_ds_64(struct shmid_ds buf)
      : shm_perm {buf.shm_perm} {
      shm_segsz = buf.shm_segsz;
      shm_atime = buf.shm_atime;
      shm_atime_high = buf.shm_atime >> 32;
      shm_dtime = buf.shm_dtime;
      shm_dtime_high = buf.shm_dtime >> 32;
      shm_ctime = buf.shm_ctime;
      shm_ctime_high = buf.shm_ctime >> 32;
      shm_cpid = buf.shm_cpid;
      shm_lpid = buf.shm_lpid;
      shm_nattch = buf.shm_nattch;
    }
  };

  static_assert(std::is_trivial<shmid_ds_64>::value, "Needs to be trivial");
  static_assert(sizeof(shmid_ds_64) == 84, "Incorrect size");

  struct semid_ds_32 {
    struct ipc_perm_32 sem_perm;
    int32_t sem_otime;
    int32_t sem_ctime;
    uint32_t sem_base;
    uint32_t sem_pending;
    uint32_t sem_pending_last;
    uint32_t undo;
    uint16_t sem_nsems;
    uint16_t _pad;

    semid_ds_32() = delete;

    operator struct semid_ds() const {
      struct semid_ds buf{};
      buf.sem_perm = sem_perm;

      buf.sem_otime = sem_otime;
      buf.sem_ctime = sem_ctime;
      buf.sem_nsems = sem_nsems;

      // sem_base, sem_pending, sem_pending_last, undo doesn't exist in the definition
      // Kernel doesn't return anything in them
      return buf;
    }

    semid_ds_32(struct semid_ds buf)
      : sem_perm {buf.sem_perm} {
      sem_otime = buf.sem_otime;
      sem_ctime = buf.sem_ctime;
      sem_nsems = buf.sem_nsems;
    }
  };

  static_assert(std::is_trivial<semid_ds_32>::value, "Needs to be trivial");
  static_assert(sizeof(semid_ds_32) == 44, "Incorrect size");

  struct semid_ds_64 {
    struct ipc_perm_64 sem_perm;
    uint32_t sem_otime;
    uint32_t sem_otime_high;
    uint32_t sem_ctime;
    uint32_t sem_ctime_high;
    uint32_t sem_nsems;
    uint32_t _pad[2];

    semid_ds_64() = delete;

    operator struct semid_ds() const {
      struct semid_ds buf{};
      buf.sem_perm = sem_perm;

      buf.sem_otime = sem_otime_high;
      buf.sem_otime <<= 32;
      buf.sem_otime |= sem_otime;
      buf.sem_ctime = sem_ctime_high;
      buf.sem_ctime <<= 32;
      buf.sem_ctime |= sem_ctime;
      buf.sem_nsems = sem_nsems;

      // sem_base, sem_pending, sem_pending_last, undo doesn't exist in the definition
      // Kernel doesn't return anything in them
      return buf;
    }

    semid_ds_64(struct semid_ds buf)
      : sem_perm {buf.sem_perm} {
      sem_otime = buf.sem_otime;
      sem_otime_high = buf.sem_otime >> 32;
      sem_ctime = buf.sem_ctime;
      sem_ctime_high = buf.sem_ctime >> 32;
      sem_nsems = buf.sem_nsems;
    }
  };

  static_assert(std::is_trivial<semid_ds_64>::value, "Needs to be trivial");
  static_assert(sizeof(semid_ds_64) == 64, "Incorrect size");

  struct msqid_ds_32 {
    struct ipc_perm_32 msg_perm;
    compat_uptr_t msg_first;
    compat_uptr_t msg_last;
    uint32_t msg_stime;
    uint32_t msg_rtime;
    uint32_t msg_ctime;
    uint32_t msg_lcbytes;
    uint32_t msg_lqbytes;
    uint16_t msg_cbytes;
    uint16_t msg_qnum;
    uint16_t msg_qbytes;
    uint16_t msg_lspid;
    uint16_t msg_lrpid;

    msqid_ds_32() = delete;
    msqid_ds_32(struct msqid_ds buf)
      : msg_perm {buf.msg_perm} {
      // msg_first and msg_last are unused and untouched
      msg_stime = buf.msg_stime;
      msg_rtime = buf.msg_rtime;
      msg_ctime = buf.msg_ctime;
      if (buf.msg_cbytes > std::numeric_limits<uint16_t>::max()) {
        msg_cbytes = std::numeric_limits<uint16_t>::max();
      }
      else {
        msg_cbytes = buf.msg_cbytes;
      }
      msg_lcbytes = buf.msg_cbytes;

      if (buf.msg_qnum > std::numeric_limits<uint16_t>::max()) {
        msg_qnum = std::numeric_limits<uint16_t>::max();
      }
      else {
        msg_qnum = buf.msg_qnum;
      }

      if (buf.msg_cbytes > std::numeric_limits<uint16_t>::max()) {
        msg_cbytes = std::numeric_limits<uint16_t>::max();
      }
      else {
        msg_cbytes = buf.msg_cbytes;
      }
      msg_lqbytes = buf.msg_qbytes;
      msg_lspid = buf.msg_lspid;
      msg_lrpid = buf.msg_lrpid;
    }
  };
  static_assert(std::is_trivial<msqid_ds_32>::value, "Needs to be trivial");
  static_assert(sizeof(msqid_ds_32) == 56, "Incorrect size");

  struct msqid_ds_64 {
    struct ipc_perm_64 msg_perm;
    uint32_t msg_stime;
    uint32_t msg_stime_high;
    uint32_t msg_rtime;
    uint32_t msg_rtime_high;
    uint32_t msg_ctime;
    uint32_t msg_ctime_high;
    uint32_t msg_cbytes;
    uint32_t msg_qnum;
    uint32_t msg_qbytes;
    uint32_t msg_lspid;
    uint32_t msg_lrpid;
    uint32_t _pad[2];

    msqid_ds_64() = delete;
    msqid_ds_64(struct msqid_ds buf)
      : msg_perm {buf.msg_perm} {
      msg_stime = buf.msg_stime;
      msg_stime_high = buf.msg_stime >> 32;
      msg_rtime = buf.msg_rtime;
      msg_rtime_high = buf.msg_rtime >> 32;
      msg_ctime = buf.msg_ctime;
      msg_ctime_high = buf.msg_ctime >> 32;
      msg_cbytes = buf.msg_cbytes;
      msg_qnum = buf.msg_qnum;
      msg_qbytes = buf.msg_qbytes;
      msg_lspid = buf.msg_lspid;
      msg_lrpid = buf.msg_lrpid;
    }
  };

  static_assert(std::is_trivial<msqid_ds_64>::value, "Needs to be trivial");
  static_assert(sizeof(msqid_ds_64) == 88, "Incorrect size");

  struct shminfo_32 {
    uint32_t shmmax;
    uint32_t shmmin;
    uint32_t shmmni;
    uint32_t shmseg;
    uint32_t shmall;

    shminfo_32() = delete;

    operator struct shminfo() const {
      struct shminfo si;
      si.shmmax = shmmax;
      si.shmmin = shmmin;
      si.shmmni = shmmni;
      si.shmseg = shmseg;
      si.shmall = shmall;
      return si;
    }

    shminfo_32(struct shminfo si) {
      shmmax = si.shmmax;
      shmmin = si.shmmin;
      shmmni = si.shmmni;
      shmseg = si.shmseg;
      shmall = si.shmall;
    }
  };

  static_assert(std::is_trivial<shminfo_32>::value, "Needs to be trivial");
  static_assert(sizeof(shminfo_32) == 20, "Incorrect size");

  struct shminfo_64 {
    compat_ulong_t shmmax;
    compat_ulong_t shmmin;
    compat_ulong_t shmmni;
    compat_ulong_t shmseg;
    compat_ulong_t shmall;
    compat_ulong_t __unused[4];

    shminfo_64() = delete;

    operator struct shminfo() const {
      struct shminfo si;
      si.shmmax = shmmax;
      si.shmmin = shmmin;
      si.shmmni = shmmni;
      si.shmseg = shmseg;
      si.shmall = shmall;
      return si;
    }

    shminfo_64(struct shminfo si) {
      shmmax = si.shmmax;
      shmmin = si.shmmin;
      shmmni = si.shmmni;
      shmseg = si.shmseg;
      shmall = si.shmall;
    }
  };

  static_assert(std::is_trivial<shminfo_64>::value, "Needs to be trivial");
  static_assert(sizeof(shminfo_64) == 36, "Incorrect size");

  struct shm_info_32 {
    int used_ids;
    uint32_t shm_tot;
    uint32_t shm_rss;
    uint32_t shm_swp;
    uint32_t swap_attempts;
    uint32_t swap_successes;

    shm_info_32() = delete;

    shm_info_32(struct shm_info si) {
      used_ids = si.used_ids;
      shm_tot = si.shm_tot;
      shm_rss = si.shm_rss;
      shm_swp = si.shm_swp;
      swap_attempts = si.swap_attempts;
      swap_successes = si.swap_successes;
    }
  };

  static_assert(std::is_trivial<shm_info_32>::value, "Needs to be trivial");
  static_assert(sizeof(shm_info_32) == 24, "Incorrect size");

  struct shm_info_64 {
    int used_ids;
    uint32_t _pad;
    uint64_t shm_tot;
    uint64_t shm_rss;
    uint64_t shm_swp;
    uint64_t swap_attempts;
    uint64_t swap_successes;

    shm_info_64() = delete;

    shm_info_64(struct shm_info si) {
      used_ids = si.used_ids;
      shm_tot = si.shm_tot;
      shm_rss = si.shm_rss;
      shm_swp = si.shm_swp;
      swap_attempts = si.swap_attempts;
      swap_successes = si.swap_successes;
    }
  };

  static_assert(std::is_trivial<shm_info_64>::value, "Needs to be trivial");
  static_assert(sizeof(shm_info_64) == 48, "Incorrect size");

  union semun_32 {
    int32_t val;      // Value for SETVAL
    compat_ptr<semid_ds_32> buf32; // struct semid_ds* - Buffer ptr for IPC_STAT, IPC_SET
    compat_ptr<semid_ds_64> buf64; // struct semid_ds* - Buffer ptr for IPC_STAT, IPC_SET
    uint32_t array;   // uint16_t array for GETALL, SETALL
    compat_ptr<struct seminfo> __buf;   // struct seminfo * - Buffer for IPC_INFO
  };

  union msgun_32 {
    int32_t val;      // Value for SETVAL
    compat_ptr<msqid_ds_32> buf32; // struct msgid_ds* - Buffer ptr for IPC_STAT, IPC_SET
    compat_ptr<msqid_ds_64> buf64; // struct msgid_ds* - Buffer ptr for IPC_STAT, IPC_SET
    uint32_t array;   // uint16_t array for GETALL, SETALL
    compat_ptr<struct msginfo> __buf;   // struct msginfo * - Buffer for IPC_INFO
  };

  union shmun_32 {
    int32_t val;      // Value for SETVAL
    compat_ptr<shmid_ds_32> buf32; // struct shmid_ds* - Buffer ptr for IPC_STAT, IPC_SET
    compat_ptr<shmid_ds_64> buf64; // struct shmid_ds* - Buffer ptr for IPC_STAT, IPC_SET
    uint32_t array;   // uint16_t array for GETALL, SETALL
    compat_ptr<struct shminfo_32> __buf32;   // struct shminfo * - Buffer for IPC_INFO
    compat_ptr<struct shminfo_64> __buf64;   // struct shminfo * - Buffer for IPC_INFO

    compat_ptr<struct shm_info_32> __buf_info_32;   // struct shm_info * - Buffer for SHM_INFO
    compat_ptr<struct shm_info_64> __buf_info_64;   // struct shm_info * - Buffer for SHM_INFO
  };

  union semun {
    int val;			/* value for SETVAL */
    struct semid_ds_32 *buf;	/* buffer for IPC_STAT & IPC_SET */
    unsigned short *array;	/* array for GETALL & SETALL */
    struct seminfo *__buf;	/* buffer for IPC_INFO */
    void *__pad;
  };

  uint64_t _ipc(FEXCore::Core::CpuStateFrame *Frame, uint32_t call, uint32_t first, uint32_t second, uint32_t third, uint32_t ptr, uint32_t fifth) {
    uint64_t Result{};

    switch (static_cast<IPCOp>(call)) {
      case OP_SEMOP: {
        Result = ::semop(first, reinterpret_cast<struct sembuf*>(ptr), second);
        break;
      }
      case OP_SEMGET: {
        Result = ::semget(first, second, third);
        break;
      }
      case OP_SEMCTL: {
        uint32_t semid = first;
        uint32_t semnum = second;
        // Upper 16bits used for a different flag?
        int32_t cmd = third & 0xFF;
        compat_ptr<semun_32> semun(ptr);
        bool IPC64 = third & 0x100;
#define UNHANDLED(x) case x: LogMan::Msg::A("Unhandled semctl cmd: " #x); break
        switch (cmd) {
          case IPC_SET: {
            struct semid_ds buf{};
            if (IPC64) {
              buf = *semun->buf64;
            }
            else {
              buf = *semun->buf32;
            }
            Result = ::semctl(semid, semnum, cmd, &buf);
            if (Result != -1) {
              if (IPC64) {
                *semun->buf64 = buf;
              }
              else {
                *semun->buf32 = buf;
              }
            }
            break;
          }
          case SEM_STAT:
          case SEM_STAT_ANY:
          case IPC_STAT: {
            struct semid_ds buf{};
            Result = ::semctl(semid, semnum, cmd, &buf);
            if (Result != -1) {
              if (IPC64) {
                *semun->buf64 = buf;
              }
              else {
                *semun->buf32 = buf;
              }
            }
            break;
          }
          case SEM_INFO:
          case IPC_INFO: {
            struct seminfo si{};
            Result = ::semctl(semid, semnum, cmd, &si);
            if (Result != -1) {
              memcpy(semun->__buf, &si, sizeof(si));
            }
            break;
          }
          case GETALL:
          case SETALL: {
            // ptr is just a int32_t* in this case
            Result = ::semctl(semid, semnum, cmd, semun->array);
            break;
          }
          case SETVAL: {
            // ptr is just a int32_t in this case
            Result = ::semctl(semid, semnum, cmd, semun->val);
            break;
          }
          case IPC_RMID:
          case GETPID:
          case GETNCNT:
          case GETZCNT:
          case GETVAL:
            Result = ::semctl(semid, semnum, cmd);
            break;
          default: LogMan::Msg::A("Unhandled semctl cmd: %d", cmd); return -EINVAL; break;
        }
#undef UNHANDLED
        break;
      }
      case OP_SEMTIMEDOP: {
        timespec32 *timeout = reinterpret_cast<timespec32*>(fifth);
        struct timespec tp64{};
        struct timespec *timed_ptr{};
        if (timeout) {
          tp64 = *timeout;
          timed_ptr = &tp64;
        }

        Result = ::semtimedop(first, reinterpret_cast<struct sembuf*>(ptr), second, timed_ptr);
        break;
      }
      case OP_MSGSND: {
        // Requires a temporary buffer
        std::vector<uint8_t> Tmp(second + sizeof(size_t));
        struct msgbuf *TmpMsg = reinterpret_cast<struct msgbuf *>(&Tmp.at(0));
        msgbuf_32 *src = reinterpret_cast<msgbuf_32*>(ptr);
        TmpMsg->mtype = src->mtype;
        memcpy(TmpMsg->mtext, src->mtext, second);

        Result = ::msgsnd(first, TmpMsg, second, third);
        break;
      }
      case OP_MSGRCV: {
        std::vector<uint8_t> Tmp(second + sizeof(size_t));
        struct msgbuf *TmpMsg = reinterpret_cast<struct msgbuf *>(&Tmp.at(0));

        Result = ::msgrcv(first, TmpMsg, second, *reinterpret_cast<uint32_t*>(fifth), third);

        if (Result != -1) {
          msgbuf_32 *src = reinterpret_cast<msgbuf_32*>(*reinterpret_cast<uint32_t*>(ptr));
          src->mtype = TmpMsg->mtype;
          memcpy(src->mtext, TmpMsg->mtext, Result);
        }
        break;
      }
      case OP_MSGGET: {
        Result = ::msgget(first, second);
        break;
      }
      case OP_MSGCTL: {
        uint32_t msqid = first;
        int32_t cmd = third & 0xFF;
        msgun_32 *msgun = reinterpret_cast<msgun_32*>(ptr);
        bool IPC64 = third & 0x100;
#define UNHANDLED(x) case x: LogMan::Msg::A("Unhandled msgctl cmd: " #x); break
        switch (cmd) {
          UNHANDLED(IPC_SET);
          case MSG_STAT:
          case MSG_STAT_ANY:
          case IPC_STAT: {
            struct msqid_ds buf{};
            Result = ::msgctl(msqid, cmd, &buf);
            if (Result != -1) {
              if (IPC64) {
                *msgun->buf64 = buf;
              }
              else {
                *msgun->buf32 = buf;
              }
            }
            break;
          }
          case MSG_INFO:
          case IPC_INFO: {
            struct msginfo mi{};
            Result = ::msgctl(msqid, cmd, reinterpret_cast<struct msqid_ds*>(&mi));
            if (Result != -1) {
              memcpy(msgun->__buf, &mi, sizeof(mi));
            }
            break;
          }
          case IPC_RMID:
            Result = ::msgctl(msqid, cmd, nullptr);
            break;
          default: LogMan::Msg::A("Unhandled msgctl cmd: %d", cmd); return -EINVAL; break;
        }
#undef UNHANDLED
        break;
      }
      case OP_SHMAT: {
        Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
          shmat(first, reinterpret_cast<const void*>(ptr), second, reinterpret_cast<uint32_t*>(third));
        break;
      }
      case OP_SHMDT: {
        Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
          shmdt(reinterpret_cast<void*>(ptr));
        break;
      }
      case OP_SHMGET: {
        Result = ::shmget(first, second, third);
        break;
      }
      case OP_SHMCTL: {
        int32_t shmid = first;
        int32_t shmcmd = second;
        int32_t cmd = shmcmd & 0xFF;
        bool IPC64 = shmcmd & 0x100;
        shmun_32 *shmun = reinterpret_cast<shmun_32*>(ptr);

        switch (cmd) {
          case IPC_SET: {
            struct shmid_ds buf{};
            if (IPC64) {
              buf = *shmun->buf64;
            }
            else {
              buf = *shmun->buf32;
            }
            Result = ::shmctl(shmid, cmd, &buf);
            if (Result != -1) {
              if (IPC64) {
                *shmun->buf64 = buf;
              }
              else {
                *shmun->buf32 = buf;
              }
            }
            break;
          }
          case SHM_STAT:
          case SHM_STAT_ANY:
          case IPC_STAT: {
            struct shmid_ds buf{};
            Result = ::shmctl(shmid, cmd, &buf);
            if (Result != -1) {
              if (IPC64) {
                buf = *shmun->buf64;
              }
              else {
                buf = *shmun->buf32;
              }
            }
            break;
          }
          case IPC_INFO: {
            struct shminfo si{};
            Result = ::shmctl(shmid, cmd, reinterpret_cast<struct shmid_ds*>(&si));
            if (Result != -1) {
              if (IPC64) {
                *shmun->__buf64 = si;
              }
              else {
                *shmun->__buf32 = si;
              }
            }
            break;
          }
          case SHM_INFO: {
            struct shm_info si{};
            Result = ::shmctl(shmid, cmd, reinterpret_cast<struct shmid_ds*>(&si));
            if (Result != -1) {
              if (IPC64) {
                *shmun->__buf_info_64 = si;
              }
              else {
                *shmun->__buf_info_32 = si;
              }
            }
            break;
          }
          case SHM_LOCK:
            Result = ::shmctl(shmid, cmd, nullptr);
            break;
          case SHM_UNLOCK:
            Result = ::shmctl(shmid, cmd, nullptr);
            break;
          case IPC_RMID:
            Result = ::shmctl(shmid, cmd, nullptr);
            break;

          default: LogMan::Msg::A("Unhandled shmctl cmd: %d", cmd); return -EINVAL; break;
        }
        break;
      }

      default: return -ENOSYS;
    }
    SYSCALL_ERRNO();
  }
  void RegisterSemaphore() {
    REGISTER_SYSCALL_IMPL_X32(ipc, _ipc);

    REGISTER_SYSCALL_IMPL_X32(semtimedop_time64, [](FEXCore::Core::CpuStateFrame *Frame, int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::semtimedop(semid, sops, nsops, timeout);
      SYSCALL_ERRNO();
    });
  }
}
