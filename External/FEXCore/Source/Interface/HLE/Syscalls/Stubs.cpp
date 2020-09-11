#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include "LogManager.h"

#include <sys/time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ptrace.h>
#include <grp.h>
#include <sys/fsuid.h>
#include <sys/capability.h>
#include <utime.h>
#include <signal.h>
#include <unistd.h>
#include <sys/timex.h>
#include <sys/resource.h>
#include <linux/aio_abi.h>
#include <mqueue.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#define SYSCALL_STUB(name) do { ERROR_AND_DIE("Syscall: " #name " stub!"); return -ENOSYS; } while(0)

namespace FEXCore::HLE {

  void RegisterStubs() {

    REGISTER_SYSCALL_IMPL(rt_sigreturn, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      SYSCALL_STUB(rt_sigreturn);
    });

    REGISTER_SYSCALL_IMPL(pause, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      SYSCALL_STUB(pause);
    });

    REGISTER_SYSCALL_IMPL(sendfile, [](FEXCore::Core::InternalThreadState *Thread, int out_fd, int in_fd, off_t *offset, size_t count) -> uint64_t {
      SYSCALL_STUB(sendfile);
    });

    REGISTER_SYSCALL_IMPL(msgget, [](FEXCore::Core::InternalThreadState *Thread, key_t key, int msgflg) -> uint64_t {
      SYSCALL_STUB(msgget);
    });

    REGISTER_SYSCALL_IMPL(msgsnd, [](FEXCore::Core::InternalThreadState *Thread, int msqid, const void *msgp, size_t msgsz, int msgflg) -> uint64_t {
      SYSCALL_STUB(msgsnd);
    });

    REGISTER_SYSCALL_IMPL(msgrcv, [](FEXCore::Core::InternalThreadState *Thread, int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg) -> uint64_t {
      SYSCALL_STUB(msgrcv);
    });

    REGISTER_SYSCALL_IMPL(msgctl, [](FEXCore::Core::InternalThreadState *Thread, int msqid, int cmd, struct msqid_ds *buf) -> uint64_t {
      SYSCALL_STUB(msgctl);
    });

    REGISTER_SYSCALL_IMPL(getrlimit, [](FEXCore::Core::InternalThreadState *Thread, int resource, struct rlimit *rlim) -> uint64_t {
      SYSCALL_STUB(getrlimit);
    });

    REGISTER_SYSCALL_IMPL(times, [](FEXCore::Core::InternalThreadState *Thread, struct tms *buf) -> uint64_t {
      SYSCALL_STUB(times);
    });

    REGISTER_SYSCALL_IMPL(ptrace, [](FEXCore::Core::InternalThreadState *Thread, int /*enum __ptrace_request*/ request, pid_t pid, void *addr, void *data) -> uint64_t {
      SYSCALL_STUB(ptrace);
    });



    REGISTER_SYSCALL_IMPL(getgroups, [](FEXCore::Core::InternalThreadState *Thread, int size, gid_t list[]) -> uint64_t {
      SYSCALL_STUB(getgroups);
    });

    REGISTER_SYSCALL_IMPL(setgroups, [](FEXCore::Core::InternalThreadState *Thread, size_t size, const gid_t *list) -> uint64_t {
      SYSCALL_STUB(setgroups);
    });

    REGISTER_SYSCALL_IMPL(capget, [](FEXCore::Core::InternalThreadState *Thread, cap_user_header_t hdrp, cap_user_data_t datap) -> uint64_t {
      SYSCALL_STUB(capget);
    });

    REGISTER_SYSCALL_IMPL(capset, [](FEXCore::Core::InternalThreadState *Thread, cap_user_header_t hdrp, const cap_user_data_t datap) -> uint64_t {
      SYSCALL_STUB(capset);
    });

    REGISTER_SYSCALL_IMPL(rt_sigtimedwait, [](FEXCore::Core::InternalThreadState *Thread, sigset_t *set, const struct timespec*, size_t sigsetsize) -> uint64_t {
      SYSCALL_STUB(rt_sigtimedwait);
    });

    REGISTER_SYSCALL_IMPL(rt_sigqueueinfo, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, int sig, siginfo_t *uinfo) -> uint64_t {
      SYSCALL_STUB(rt_sigqueueinfo);
    });
    REGISTER_SYSCALL_IMPL(utime, [](FEXCore::Core::InternalThreadState *Thread, char* filename, const struct utimbuf* times) -> uint64_t {
      SYSCALL_STUB(utime);
    });

    REGISTER_SYSCALL_IMPL(ustat, [](FEXCore::Core::InternalThreadState *Thread, dev_t dev, struct ustat *ubuf) -> uint64_t {
      SYSCALL_STUB(ustat);
    });

    /*
      arg1 is one of: void, unsigned int fs_index, const char *fsname
      arg2 is one of: void, char *buf
    */
    REGISTER_SYSCALL_IMPL(sysfs, [](FEXCore::Core::InternalThreadState *Thread, int option,  uint64_t arg1,  uint64_t arg2) -> uint64_t {
      SYSCALL_STUB(sysfs);
    });

    REGISTER_SYSCALL_IMPL(vhangup, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      SYSCALL_STUB(vhangup);
    });

    REGISTER_SYSCALL_IMPL(modify_ldt, [](FEXCore::Core::InternalThreadState *Thread, int func, void *ptr, unsigned long bytecount) -> uint64_t {
      SYSCALL_STUB(modify_ldt);
    });

    REGISTER_SYSCALL_IMPL(pivot_root, [](FEXCore::Core::InternalThreadState *Thread, const char *new_root, const char *put_old) -> uint64_t {
      SYSCALL_STUB(pivot_root);
    });

    REGISTER_SYSCALL_IMPL(adjtimex, [](FEXCore::Core::InternalThreadState *Thread, struct timex *buf) -> uint64_t {
      SYSCALL_STUB(adjtimex);
    });

    REGISTER_SYSCALL_IMPL(setrlimit, [](FEXCore::Core::InternalThreadState *Thread, int resource, const struct rlimit *rlim) -> uint64_t {
      SYSCALL_STUB(setrlimit);
    });

    REGISTER_SYSCALL_IMPL(chroot, [](FEXCore::Core::InternalThreadState *Thread, const char *path) -> uint64_t {
      SYSCALL_STUB(chroot);
    });

    REGISTER_SYSCALL_IMPL(acct, [](FEXCore::Core::InternalThreadState *Thread, const char *filename) -> uint64_t {
      SYSCALL_STUB(acct);
    });

    REGISTER_SYSCALL_IMPL(settimeofday, [](FEXCore::Core::InternalThreadState *Thread, const struct timeval *tv, const struct timezone *tz) -> uint64_t {
      SYSCALL_STUB(settimeofday);
    });

    REGISTER_SYSCALL_IMPL(mount, [](FEXCore::Core::InternalThreadState *Thread, const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) -> uint64_t {
      SYSCALL_STUB(mount);
    });

    REGISTER_SYSCALL_IMPL(umount2, [](FEXCore::Core::InternalThreadState *Thread, const char *target, int flags) -> uint64_t {
      SYSCALL_STUB(umount2);
    });

    REGISTER_SYSCALL_IMPL(swapon, [](FEXCore::Core::InternalThreadState *Thread, const char *path, int swapflags) -> uint64_t {
      SYSCALL_STUB(swapon);
    });

    REGISTER_SYSCALL_IMPL(swapoff, [](FEXCore::Core::InternalThreadState *Thread, const char *path) -> uint64_t {
      SYSCALL_STUB(swapoff);
    });

    REGISTER_SYSCALL_IMPL(reboot, [](FEXCore::Core::InternalThreadState *Thread, int magic, int magic2, int cmd, void *arg) -> uint64_t {
      SYSCALL_STUB(reboot);
    });

    REGISTER_SYSCALL_IMPL(sethostname, [](FEXCore::Core::InternalThreadState *Thread, const char *name, size_t len) -> uint64_t {
      SYSCALL_STUB(sethostname);
    });

    REGISTER_SYSCALL_IMPL(setdomainname, [](FEXCore::Core::InternalThreadState *Thread, const char *name, size_t len) -> uint64_t {
      SYSCALL_STUB(setdomainname);
    });

    REGISTER_SYSCALL_IMPL(iopl, [](FEXCore::Core::InternalThreadState *Thread, int level) -> uint64_t {
      SYSCALL_STUB(iopl);
    });

    REGISTER_SYSCALL_IMPL(init_module, [](FEXCore::Core::InternalThreadState *Thread, void *module_image, unsigned long len, const char *param_values) -> uint64_t {
      SYSCALL_STUB(init_module);
    });

    REGISTER_SYSCALL_IMPL(delete_module, [](FEXCore::Core::InternalThreadState *Thread, const char *name, int flags) -> uint64_t {
      SYSCALL_STUB(delete_module);
    });

    REGISTER_SYSCALL_IMPL(quotactl, [](FEXCore::Core::InternalThreadState *Thread, int cmd, const char *special, int id, caddr_t addr) -> uint64_t {
      SYSCALL_STUB(quotactl);
    });

    REGISTER_SYSCALL_IMPL(readahead, [](FEXCore::Core::InternalThreadState *Thread, int fd, off64_t offset, size_t count) -> uint64_t {
      SYSCALL_STUB(readahead);
    });

    REGISTER_SYSCALL_IMPL(io_setup, [](FEXCore::Core::InternalThreadState *Thread, unsigned nr_events, aio_context_t *ctx_idp) -> uint64_t {
      SYSCALL_STUB(io_setup);
    });

    REGISTER_SYSCALL_IMPL(io_destroy, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id) -> uint64_t {
      SYSCALL_STUB(io_destroy);
    });

    REGISTER_SYSCALL_IMPL(io_getevents, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout) -> uint64_t {
      SYSCALL_STUB(io_getevents);
    });

    REGISTER_SYSCALL_IMPL(io_submit, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, long nr, struct iocb **iocbpp) -> uint64_t {
      SYSCALL_STUB(io_submit);
    });

    REGISTER_SYSCALL_IMPL(io_cancel, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, struct iocb *iocb, struct io_event *result) -> uint64_t {
      SYSCALL_STUB(io_cancel);
    });

    REGISTER_SYSCALL_IMPL(lookup_dcookie, [](FEXCore::Core::InternalThreadState *Thread, uint64_t cookie, char *buffer, size_t len) -> uint64_t {
      SYSCALL_STUB(lookup_dcookie);
    });

    REGISTER_SYSCALL_IMPL(remap_file_pages, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t size, int prot, size_t pgoff, int flags) -> uint64_t {
      SYSCALL_STUB(remap_file_pages);
    });

    REGISTER_SYSCALL_IMPL(restart_syscall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      SYSCALL_STUB(restart_syscall);
    });

    REGISTER_SYSCALL_IMPL(clock_settime, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, const struct timespec *tp) -> uint64_t {
      SYSCALL_STUB(clock_settime);
    });

    REGISTER_SYSCALL_IMPL(utimes, [](FEXCore::Core::InternalThreadState *Thread, const char *filename, const struct timeval times[2]) -> uint64_t {
      SYSCALL_STUB(utimes);
    });

    REGISTER_SYSCALL_IMPL(mbind, [](FEXCore::Core::InternalThreadState *Thread, void *addr, unsigned long len, int mode, const unsigned long *nodemask, unsigned long maxnode, unsigned flags) -> uint64_t {
      SYSCALL_STUB(mbind);
    });

    REGISTER_SYSCALL_IMPL(set_mempolicy, [](FEXCore::Core::InternalThreadState *Thread, int mode, const unsigned long *nodemask, unsigned long maxnode) -> uint64_t {
      SYSCALL_STUB(set_mempolicy);
    });

    // last two parameters are optional
    REGISTER_SYSCALL_IMPL(mq_open, [](FEXCore::Core::InternalThreadState *Thread, const char *name, int oflag, mode_t mode, struct mq_attr *attr) -> uint64_t {
      SYSCALL_STUB(mq_open);
    });

    REGISTER_SYSCALL_IMPL(mq_unlink, [](FEXCore::Core::InternalThreadState *Thread, const char *name) -> uint64_t {
      SYSCALL_STUB(mq_unlink);
    });

    REGISTER_SYSCALL_IMPL(mq_timedsend, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      SYSCALL_STUB(mq_timedsend);
    });

    REGISTER_SYSCALL_IMPL(mq_timedreceive, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      SYSCALL_STUB(mq_timedreceive);
    });

    REGISTER_SYSCALL_IMPL(mq_notify, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, const struct sigevent *sevp) -> uint64_t {
      SYSCALL_STUB(mq_notify);
    });

    REGISTER_SYSCALL_IMPL(mq_getsetattr, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr) -> uint64_t {
      SYSCALL_STUB(mq_getsetattr);
    });

    REGISTER_SYSCALL_IMPL(kexec_load, [](FEXCore::Core::InternalThreadState *Thread, unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags) -> uint64_t {
      SYSCALL_STUB(kexec_load);
    });

    REGISTER_SYSCALL_IMPL(waitid, [](FEXCore::Core::InternalThreadState *Thread, int /*idtype_t*/ idtype, id_t id, siginfo_t *infop, int options) -> uint64_t {
      SYSCALL_STUB(waitid);
    });

    REGISTER_SYSCALL_IMPL(add_key, [](FEXCore::Core::InternalThreadState *Thread, const char *type, const char *description, const void *payload, size_t plen, int32_t /*key_serial_t*/ keyring) -> uint64_t {
      SYSCALL_STUB(add_key);
    });

    REGISTER_SYSCALL_IMPL(request_key, [](FEXCore::Core::InternalThreadState *Thread, const char *type, const char *description, const char *callout_info, int32_t /*key_serial_t*/ dest_keyring) -> uint64_t {
      SYSCALL_STUB(request_key);
    });

    REGISTER_SYSCALL_IMPL(keyctl, [](FEXCore::Core::InternalThreadState *Thread, int operation, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) -> uint64_t {
      SYSCALL_STUB(keyctl);
    });

    REGISTER_SYSCALL_IMPL(ioprio_set, [](FEXCore::Core::InternalThreadState *Thread, int which, int who) -> uint64_t {
      SYSCALL_STUB(ioprio_set);
    });

    REGISTER_SYSCALL_IMPL(ioprio_get, [](FEXCore::Core::InternalThreadState *Thread, int which, int who, int ioprio) -> uint64_t {
      SYSCALL_STUB(ioprio_get);
    });

    REGISTER_SYSCALL_IMPL(migrate_pages, [](FEXCore::Core::InternalThreadState *Thread, int pid, unsigned long maxnode, const unsigned long *old_nodes, const unsigned long *new_nodes) -> uint64_t {
      SYSCALL_STUB(migrate_pages);
    });

    REGISTER_SYSCALL_IMPL(mkdirat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, mode_t mode) -> uint64_t {
      SYSCALL_STUB(mkdirat);
    });

    REGISTER_SYSCALL_IMPL(mknodat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, mode_t mode, dev_t dev) -> uint64_t {
      SYSCALL_STUB(mknodat);
    });

    REGISTER_SYSCALL_IMPL(fchownat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) -> uint64_t {
      SYSCALL_STUB(fchownat);
    });

    REGISTER_SYSCALL_IMPL(futimesat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, const struct timeval times[2]) -> uint64_t {
      SYSCALL_STUB(futimesat);
    });

    REGISTER_SYSCALL_IMPL(unlinkat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, int flags) -> uint64_t {
      SYSCALL_STUB(unlinkat);
    });

    REGISTER_SYSCALL_IMPL(renameat, [](FEXCore::Core::InternalThreadState *Thread, int olddirfd, const char *oldpath, int newdirfd, const char *newpath) -> uint64_t {
      SYSCALL_STUB(renameat);
    });

    REGISTER_SYSCALL_IMPL(linkat, [](FEXCore::Core::InternalThreadState *Thread, int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) -> uint64_t {
      SYSCALL_STUB(linkat);
    });

    REGISTER_SYSCALL_IMPL(symlinkat, [](FEXCore::Core::InternalThreadState *Thread, const char *target, int newdirfd, const char *linkpath) -> uint64_t {
      SYSCALL_STUB(symlinkat);
    });

    REGISTER_SYSCALL_IMPL(fchmodat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, mode_t mode, int flags) -> uint64_t {
      SYSCALL_STUB(fchmodat);
    });

    REGISTER_SYSCALL_IMPL(unshare, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      SYSCALL_STUB(unshare);
    });

    REGISTER_SYSCALL_IMPL(splice, [](FEXCore::Core::InternalThreadState *Thread, int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags) -> uint64_t {
      SYSCALL_STUB(splice);
    });

    REGISTER_SYSCALL_IMPL(tee, [](FEXCore::Core::InternalThreadState *Thread, int fd_in, int fd_out, size_t len, unsigned int flags) -> uint64_t {
      SYSCALL_STUB(tee);
    });

    REGISTER_SYSCALL_IMPL(sync_file_range, [](FEXCore::Core::InternalThreadState *Thread, int fd, off64_t offset, off64_t nbytes, unsigned int flags) -> uint64_t {
      SYSCALL_STUB(sync_file_range);
    });

    REGISTER_SYSCALL_IMPL(vmsplice, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, unsigned long nr_segs, unsigned int flags) -> uint64_t {
      SYSCALL_STUB(vmsplice);
    });

    REGISTER_SYSCALL_IMPL(move_pages, [](FEXCore::Core::InternalThreadState *Thread, int pid, unsigned long count, void **pages, const int *nodes, int *status, int flags) -> uint64_t {
      SYSCALL_STUB(move_pages);
    });

    REGISTER_SYSCALL_IMPL(utimensat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, const struct timespec times[2], int flags) -> uint64_t {
      SYSCALL_STUB(utimensat);
    });

    REGISTER_SYSCALL_IMPL(signalfd, [](FEXCore::Core::InternalThreadState *Thread, int fd, const sigset_t *mask, size_t sizemask) -> uint64_t {
      SYSCALL_STUB(signalfd);
    });

    REGISTER_SYSCALL_IMPL(signalfd4, [](FEXCore::Core::InternalThreadState *Thread, int fd, const sigset_t *mask, size_t sizemask, int flags) -> uint64_t {
      SYSCALL_STUB(signalfd4);
    });

    REGISTER_SYSCALL_IMPL(preadv, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset) -> uint64_t {
      SYSCALL_STUB(preadv);
    });

    REGISTER_SYSCALL_IMPL(pwritev, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset) -> uint64_t {
      SYSCALL_STUB(pwritev);
    });

    REGISTER_SYSCALL_IMPL(rt_tgsigqueueinfo, [](FEXCore::Core::InternalThreadState *Thread, pid_t tgid, pid_t tid, int sig, siginfo_t *info) -> uint64_t {
      SYSCALL_STUB(rt_tgsigqueueinfo);
    });

    REGISTER_SYSCALL_IMPL(perf_event_open, [](FEXCore::Core::InternalThreadState *Thread, struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) -> uint64_t {
      SYSCALL_STUB(perf_event_open);
    });

    REGISTER_SYSCALL_IMPL(recvmmsg, [](FEXCore::Core::InternalThreadState *Thread, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout) -> uint64_t {
      SYSCALL_STUB(recvmmsg);
    });

    REGISTER_SYSCALL_IMPL(fanotify_init, [](FEXCore::Core::InternalThreadState *Thread, unsigned int flags, unsigned int event_f_flags) -> uint64_t {
      SYSCALL_STUB(fanotify_init);
    });

    REGISTER_SYSCALL_IMPL(fanotify_mark, [](FEXCore::Core::InternalThreadState *Thread, int fanotify_fd, unsigned int flags, uint64_t mask, int dirfd, const char *pathname) -> uint64_t {
      SYSCALL_STUB(fanotify_mark);
    });

    REGISTER_SYSCALL_IMPL(clock_adjtime, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timex *buf) -> uint64_t {
      SYSCALL_STUB(clock_adjtime);
    });

    REGISTER_SYSCALL_IMPL(setns, [](FEXCore::Core::InternalThreadState *Thread, int fd, int nstype) -> uint64_t {
      SYSCALL_STUB(setns);
    });

    REGISTER_SYSCALL_IMPL(getcpu, [](FEXCore::Core::InternalThreadState *Thread, unsigned *cpu, unsigned *node, struct getcpu_cache *tcache) -> uint64_t {
      SYSCALL_STUB(getcpu);
    });

    REGISTER_SYSCALL_IMPL(process_vm_readv, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct iovec *local_iov, unsigned long liovcnt, const struct iovec *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      SYSCALL_STUB(process_vm_readv);
    });

    REGISTER_SYSCALL_IMPL(process_vm_writev, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, const struct iovec *local_iov, unsigned long liovcnt, const struct iovec *remote_iov, unsigned long riovcnt, unsigned long flags) -> uint64_t {
      SYSCALL_STUB(process_vm_writev);
    });

    //compare  two  processes  to determine if they share a kernel resource
    REGISTER_SYSCALL_IMPL(kcmp, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid1, pid_t pid2, int type, unsigned long idx1, unsigned long idx2) -> uint64_t {
      SYSCALL_STUB(kcmp);
    });

    REGISTER_SYSCALL_IMPL(finit_module, [](FEXCore::Core::InternalThreadState *Thread, int fd, const char *param_values, int flags) -> uint64_t {
      SYSCALL_STUB(finit_module);
    });

    REGISTER_SYSCALL_IMPL(renameat2, [](FEXCore::Core::InternalThreadState *Thread, int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) -> uint64_t {
      SYSCALL_STUB(renameat2);
    });

    REGISTER_SYSCALL_IMPL(seccomp, [](FEXCore::Core::InternalThreadState *Thread, unsigned int operation, unsigned int flags, void *args) -> uint64_t {
      SYSCALL_STUB(seccomp);
    });

    REGISTER_SYSCALL_IMPL(kexec_file_load, [](FEXCore::Core::InternalThreadState *Thread, int kernel_fd, int initrd_fd, unsigned long cmdline_len, const char *cmdline, unsigned long flags) -> uint64_t {
      SYSCALL_STUB(kexec_file_load);
    });

    REGISTER_SYSCALL_IMPL(bpf, [](FEXCore::Core::InternalThreadState *Thread, int cmd, union bpf_attr *attr, unsigned int size) -> uint64_t {
      SYSCALL_STUB(bpf);
    });

    // execute program relative to a directory file descriptor
    REGISTER_SYSCALL_IMPL(execveat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, char *const argv[], char *const envp[], int flags) -> uint64_t {
      SYSCALL_STUB(execveat);
    });

    REGISTER_SYSCALL_IMPL(userfaultfd, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      SYSCALL_STUB(userfaultfd);
    });

    REGISTER_SYSCALL_IMPL(membarrier, [](FEXCore::Core::InternalThreadState *Thread, int cmd, int flags) -> uint64_t {
      SYSCALL_STUB(membarrier);
    });

    REGISTER_SYSCALL_IMPL(copy_file_range, [](FEXCore::Core::InternalThreadState *Thread, int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags) -> uint64_t {
      SYSCALL_STUB(copy_file_range);
    });

    REGISTER_SYSCALL_IMPL(preadv2, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags) -> uint64_t {
      SYSCALL_STUB(preadv2);
    });

    REGISTER_SYSCALL_IMPL(pwritev2, [](FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags) -> uint64_t {
      SYSCALL_STUB(pwritev2);
    });

    REGISTER_SYSCALL_IMPL(pkey_mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot, int pkey) -> uint64_t {
      SYSCALL_STUB(pkey_mprotect);
    });

    REGISTER_SYSCALL_IMPL(pkey_alloc, [](FEXCore::Core::InternalThreadState *Thread, unsigned int flags, unsigned int access_rights) -> uint64_t {
      SYSCALL_STUB(pkey_alloc);
    });

    REGISTER_SYSCALL_IMPL(pkey_free, [](FEXCore::Core::InternalThreadState *Thread, int pkey) -> uint64_t {
      SYSCALL_STUB(pkey_free);
    });

    REGISTER_SYSCALL_IMPL(io_pgetevents, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout, const struct io_sigset  *usig) -> uint64_t {
      SYSCALL_STUB(io_pgetevents);
    });

    REGISTER_SYSCALL_IMPL(rseq, [](FEXCore::Core::InternalThreadState *Thread,  struct rseq  *rseq, uint32_t rseq_len, int flags, uint32_t sig) -> uint64_t {
      SYSCALL_STUB(rseq);
    });
  }
}
