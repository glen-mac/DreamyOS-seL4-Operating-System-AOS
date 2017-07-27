/* @LICENSE(NICTA) */

#ifndef _SYSCALL_STUBS_SEL4_ARM_H_
#define _SYSCALL_STUBS_SEL4_ARM_H_

#include <stdarg.h>
#include <syscall_sel4.h>

long sys_restart_syscall(va_list ap);
long sys_exit(va_list ap);
long sys_fork(va_list ap);
long sys_read(va_list ap);
long sys_write(va_list ap);
long sys_open(va_list ap);
long sys_close(va_list ap);
long sys_creat(va_list ap);
long sys_link(va_list ap);
long sys_unlink(va_list ap);
long sys_execve(va_list ap);
long sys_chdir(va_list ap);
long sys_mknod(va_list ap);
long sys_chmod(va_list ap);
long sys_lchown(va_list ap);
long sys_lseek(va_list ap);
long sys_getpid(va_list ap);
long sys_mount(va_list ap);
long sys_setuid(va_list ap);
long sys_getuid(va_list ap);
long sys_ptrace(va_list ap);
long sys_pause(va_list ap);
long sys_access(va_list ap);
long sys_nice(va_list ap);
long sys_sync(va_list ap);
long sys_kill(va_list ap);
long sys_rename(va_list ap);
long sys_mkdir(va_list ap);
long sys_rmdir(va_list ap);
long sys_dup(va_list ap);
long sys_pipe(va_list ap);
long sys_times(va_list ap);
long sys_brk(va_list ap);
long sys_setgid(va_list ap);
long sys_getgid(va_list ap);
long sys_geteuid(va_list ap);
long sys_getegid(va_list ap);
long sys_acct(va_list ap);
long sys_umount2(va_list ap);
long sys_ioctl(va_list ap);
long sys_fcntl(va_list ap);
long sys_setpgid(va_list ap);
long sys_umask(va_list ap);
long sys_chroot(va_list ap);
long sys_ustat(va_list ap);
long sys_dup2(va_list ap);
long sys_getppid(va_list ap);
long sys_getpgrp(va_list ap);
long sys_setsid(va_list ap);
long sys_sigaction(va_list ap);
long sys_setreuid(va_list ap);
long sys_setregid(va_list ap);
long sys_sigsuspend(va_list ap);
long sys_sigpending(va_list ap);
long sys_sethostname(va_list ap);
long sys_setrlimit(va_list ap);
long sys_getrusage(va_list ap);
long sys_gettimeofday(va_list ap);
long sys_settimeofday(va_list ap);
long sys_getgroups(va_list ap);
long sys_setgroups(va_list ap);
long sys_symlink(va_list ap);
long sys_readlink(va_list ap);
long sys_uselib(va_list ap);
long sys_swapon(va_list ap);
long sys_reboot(va_list ap);
long sys_munmap(va_list ap);
long sys_truncate(va_list ap);
long sys_ftruncate(va_list ap);
long sys_fchmod(va_list ap);
long sys_fchown(va_list ap);
long sys_getpriority(va_list ap);
long sys_setpriority(va_list ap);
long sys_statfs(va_list ap);
long sys_fstatfs(va_list ap);
long sys_syslog(va_list ap);
long sys_setitimer(va_list ap);
long sys_getitimer(va_list ap);
long sys_stat(va_list ap);
long sys_lstat(va_list ap);
long sys_fstat(va_list ap);
long sys_vhangup(va_list ap);
long sys_wait4(va_list ap);
long sys_swapoff(va_list ap);
long sys_sysinfo(va_list ap);
long sys_fsync(va_list ap);
long sys_sigreturn(va_list ap);
long sys_clone(va_list ap);
long sys_setdomainname(va_list ap);
long sys_uname(va_list ap);
long sys_adjtimex(va_list ap);
long sys_mprotect(va_list ap);
long sys_sigprocmask(va_list ap);
long sys_init_module(va_list ap);
long sys_delete_module(va_list ap);
long sys_quotactl(va_list ap);
long sys_getpgid(va_list ap);
long sys_fchdir(va_list ap);
long sys_bdflush(va_list ap);
long sys_sysfs(va_list ap);
long sys_personality(va_list ap);
long sys_setfsuid(va_list ap);
long sys_setfsgid(va_list ap);
long sys_getdents(va_list ap);
long sys__newselect(va_list ap);
long sys_flock(va_list ap);
long sys_msync(va_list ap);
long sys_readv(va_list ap);
long sys_writev(va_list ap);
long sys_getsid(va_list ap);
long sys_fdatasync(va_list ap);
long sys__sysctl(va_list ap);
long sys_mlock(va_list ap);
long sys_munlock(va_list ap);
long sys_mlockall(va_list ap);
long sys_munlockall(va_list ap);
long sys_sched_setparam(va_list ap);
long sys_sched_getparam(va_list ap);
long sys_sched_setscheduler(va_list ap);
long sys_sched_getscheduler(va_list ap);
long sys_sched_yield(va_list ap);
long sys_sched_get_priority_max(va_list ap);
long sys_sched_get_priority_min(va_list ap);
long sys_sched_rr_get_interval(va_list ap);
long sys_nanosleep(va_list ap);
long sys_mremap(va_list ap);
long sys_setresuid(va_list ap);
long sys_getresuid(va_list ap);
long sys_poll(va_list ap);
long sys_nfsservctl(va_list ap);
long sys_setresgid(va_list ap);
long sys_getresgid(va_list ap);
long sys_prctl(va_list ap);
long sys_rt_sigreturn(va_list ap);
long sys_rt_sigaction(va_list ap);
long sys_rt_sigprocmask(va_list ap);
long sys_rt_sigpending(va_list ap);
long sys_rt_sigtimedwait(va_list ap);
long sys_rt_sigqueueinfo(va_list ap);
long sys_rt_sigsuspend(va_list ap);
long sys_pread64(va_list ap);
long sys_pwrite64(va_list ap);
long sys_chown(va_list ap);
long sys_getcwd(va_list ap);
long sys_capget(va_list ap);
long sys_capset(va_list ap);
long sys_sigaltstack(va_list ap);
long sys_sendfile(va_list ap);
long sys_vfork(va_list ap);
long sys_ugetrlimit(va_list ap);
long sys_mmap2(va_list ap);
long sys_truncate64(va_list ap);
long sys_ftruncate64(va_list ap);
long sys_stat64(va_list ap);
long sys_lstat64(va_list ap);
long sys_fstat64(va_list ap);
long sys_lchown32(va_list ap);
long sys_getuid32(va_list ap);
long sys_getgid32(va_list ap);
long sys_geteuid32(va_list ap);
long sys_getegid32(va_list ap);
long sys_setreuid32(va_list ap);
long sys_setregid32(va_list ap);
long sys_getgroups32(va_list ap);
long sys_setgroups32(va_list ap);
long sys_fchown32(va_list ap);
long sys_setresuid32(va_list ap);
long sys_getresuid32(va_list ap);
long sys_setresgid32(va_list ap);
long sys_getresgid32(va_list ap);
long sys_chown32(va_list ap);
long sys_setuid32(va_list ap);
long sys_setgid32(va_list ap);
long sys_setfsuid32(va_list ap);
long sys_setfsgid32(va_list ap);
long sys_getdents64(va_list ap);
long sys_pivot_root(va_list ap);
long sys_mincore(va_list ap);
long sys_madvise(va_list ap);
long sys_fcntl64(va_list ap);
long sys_gettid(va_list ap);
long sys_readahead(va_list ap);
long sys_setxattr(va_list ap);
long sys_lsetxattr(va_list ap);
long sys_fsetxattr(va_list ap);
long sys_getxattr(va_list ap);
long sys_lgetxattr(va_list ap);
long sys_fgetxattr(va_list ap);
long sys_listxattr(va_list ap);
long sys_llistxattr(va_list ap);
long sys_flistxattr(va_list ap);
long sys_removexattr(va_list ap);
long sys_lremovexattr(va_list ap);
long sys_fremovexattr(va_list ap);
long sys_tkill(va_list ap);
long sys_sendfile64(va_list ap);
long sys_futex(va_list ap);
long sys_sched_setaffinity(va_list ap);
long sys_sched_getaffinity(va_list ap);
long sys_io_setup(va_list ap);
long sys_io_destroy(va_list ap);
long sys_io_getevents(va_list ap);
long sys_io_submit(va_list ap);
long sys_io_cancel(va_list ap);
long sys_exit_group(va_list ap);
long sys_lookup_dcookie(va_list ap);
long sys_epoll_create(va_list ap);
long sys_epoll_ctl(va_list ap);
long sys_epoll_wait(va_list ap);
long sys_remap_file_pages(va_list ap);
long sys_set_tid_address(va_list ap);
long sys_timer_create(va_list ap);
long sys_timer_settime(va_list ap);
long sys_timer_gettime(va_list ap);
long sys_timer_getoverrun(va_list ap);
long sys_timer_delete(va_list ap);
long sys_clock_settime(va_list ap);
long sys_clock_gettime(va_list ap);
long sys_clock_getres(va_list ap);
long sys_clock_nanosleep(va_list ap);
long sys_statfs64(va_list ap);
long sys_fstatfs64(va_list ap);
long sys_tgkill(va_list ap);
long sys_utimes(va_list ap);
long sys_fadvise64_64(va_list ap);
long sys_pciconfig_iobase(va_list ap);
long sys_pciconfig_read(va_list ap);
long sys_pciconfig_write(va_list ap);
long sys_mq_open(va_list ap);
long sys_mq_unlink(va_list ap);
long sys_mq_timedsend(va_list ap);
long sys_mq_timedreceive(va_list ap);
long sys_mq_notify(va_list ap);
long sys_mq_getsetattr(va_list ap);
long sys_waitid(va_list ap);
long sys_socket(va_list ap);
long sys_bind(va_list ap);
long sys_connect(va_list ap);
long sys_listen(va_list ap);
long sys_accept(va_list ap);
long sys_getsockname(va_list ap);
long sys_getpeername(va_list ap);
long sys_socketpair(va_list ap);
long sys_send(va_list ap);
long sys_sendto(va_list ap);
long sys_recv(va_list ap);
long sys_recvfrom(va_list ap);
long sys_shutdown(va_list ap);
long sys_setsockopt(va_list ap);
long sys_getsockopt(va_list ap);
long sys_sendmsg(va_list ap);
long sys_recvmsg(va_list ap);
long sys_semop(va_list ap);
long sys_semget(va_list ap);
long sys_semctl(va_list ap);
long sys_msgsnd(va_list ap);
long sys_msgrcv(va_list ap);
long sys_msgget(va_list ap);
long sys_msgctl(va_list ap);
long sys_shmat(va_list ap);
long sys_shmdt(va_list ap);
long sys_shmget(va_list ap);
long sys_shmctl(va_list ap);
long sys_add_key(va_list ap);
long sys_request_key(va_list ap);
long sys_keyctl(va_list ap);
long sys_semtimedop(va_list ap);
long sys_vserver(va_list ap);
long sys_ioprio_set(va_list ap);
long sys_ioprio_get(va_list ap);
long sys_inotify_init(va_list ap);
long sys_inotify_add_watch(va_list ap);
long sys_inotify_rm_watch(va_list ap);
long sys_mbind(va_list ap);
long sys_get_mempolicy(va_list ap);
long sys_set_mempolicy(va_list ap);
long sys_openat(va_list ap);
long sys_mkdirat(va_list ap);
long sys_mknodat(va_list ap);
long sys_fchownat(va_list ap);
long sys_futimesat(va_list ap);
long sys_fstatat64(va_list ap);
long sys_unlinkat(va_list ap);
long sys_renameat(va_list ap);
long sys_linkat(va_list ap);
long sys_symlinkat(va_list ap);
long sys_readlinkat(va_list ap);
long sys_fchmodat(va_list ap);
long sys_faccessat(va_list ap);
long sys_pselect6(va_list ap);
long sys_ppoll(va_list ap);
long sys_unshare(va_list ap);
long sys_set_robust_list(va_list ap);
long sys_get_robust_list(va_list ap);
long sys_splice(va_list ap);
long sys_sync_file_range2(va_list ap);
long sys_tee(va_list ap);
long sys_vmsplice(va_list ap);
long sys_move_pages(va_list ap);
long sys_getcpu(va_list ap);
long sys_epoll_pwait(va_list ap);
long sys_kexec_load(va_list ap);
long sys_utimensat(va_list ap);
long sys_signalfd(va_list ap);
long sys_timerfd_create(va_list ap);
long sys_eventfd(va_list ap);
long sys_fallocate(va_list ap);
long sys_timerfd_settime(va_list ap);
long sys_timerfd_gettime(va_list ap);
long sys_signalfd4(va_list ap);
long sys_eventfd2(va_list ap);
long sys_epoll_create1(va_list ap);
long sys_dup3(va_list ap);
long sys_pipe2(va_list ap);
long sys_inotify_init1(va_list ap);
long sys_preadv(va_list ap);
long sys_pwritev(va_list ap);
long sys_rt_tgsigqueueinfo(va_list ap);
long sys_perf_event_open(va_list ap);
long sys_recvmmsg(va_list ap);
long sys_accept4(va_list ap);
long sys_fanotify_init(va_list ap);
long sys_fanotify_mark(va_list ap);
long sys_prlimit64(va_list ap);
long sys_name_to_handle_at(va_list ap);
long sys_open_by_handle_at(va_list ap);
long sys_clock_adjtime(va_list ap);
long sys_syncfs(va_list ap);
long sys_sendmmsg(va_list ap);
long sys_setns(va_list ap);
long sys_process_vm_readv(va_list ap);
long sys_process_vm_writev(va_list ap);

#define SET_MUSLC_SYSCALL_TABLE \
    do { \
        static muslc_syscall_t __muslc_syscall_ptr_table_impl[SYSCALL_MUSLC_NUM] = { \
            sys_restart_syscall, /* 0 */ \
            sys_exit, /* 1 */ \
            sys_fork, /* 2 */ \
            sys_read, /* 3 */ \
            sys_write, /* 4 */ \
            sys_open, /* 5 */ \
            sys_close, /* 6 */ \
            NULL, /* 7 */ \
            sys_creat, /* 8 */ \
            sys_link, /* 9 */ \
            sys_unlink, /* 10 */ \
            sys_execve, /* 11 */ \
            sys_chdir, /* 12 */ \
            NULL, /* 13 */ \
            sys_mknod, /* 14 */ \
            sys_chmod, /* 15 */ \
            sys_lchown, /* 16 */ \
            NULL, /* 17 */ \
            NULL, /* 18 */ \
            sys_lseek, /* 19 */ \
            sys_getpid, /* 20 */ \
            sys_mount, /* 21 */ \
            NULL, /* 22 */ \
            sys_setuid, /* 23 */ \
            sys_getuid, /* 24 */ \
            NULL, /* 25 */ \
            sys_ptrace, /* 26 */ \
            NULL, /* 27 */ \
            NULL, /* 28 */ \
            sys_pause, /* 29 */ \
            NULL, /* 30 */ \
            NULL, /* 31 */ \
            NULL, /* 32 */ \
            sys_access, /* 33 */ \
            sys_nice, /* 34 */ \
            NULL, /* 35 */ \
            sys_sync, /* 36 */ \
            sys_kill, /* 37 */ \
            sys_rename, /* 38 */ \
            sys_mkdir, /* 39 */ \
            sys_rmdir, /* 40 */ \
            sys_dup, /* 41 */ \
            sys_pipe, /* 42 */ \
            sys_times, /* 43 */ \
            NULL, /* 44 */ \
            sys_brk, /* 45 */ \
            sys_setgid, /* 46 */ \
            sys_getgid, /* 47 */ \
            NULL, /* 48 */ \
            sys_geteuid, /* 49 */ \
            sys_getegid, /* 50 */ \
            sys_acct, /* 51 */ \
            sys_umount2, /* 52 */ \
            NULL, /* 53 */ \
            sys_ioctl, /* 54 */ \
            sys_fcntl, /* 55 */ \
            NULL, /* 56 */ \
            sys_setpgid, /* 57 */ \
            NULL, /* 58 */ \
            NULL, /* 59 */ \
            sys_umask, /* 60 */ \
            sys_chroot, /* 61 */ \
            sys_ustat, /* 62 */ \
            sys_dup2, /* 63 */ \
            sys_getppid, /* 64 */ \
            sys_getpgrp, /* 65 */ \
            sys_setsid, /* 66 */ \
            sys_sigaction, /* 67 */ \
            NULL, /* 68 */ \
            NULL, /* 69 */ \
            sys_setreuid, /* 70 */ \
            sys_setregid, /* 71 */ \
            sys_sigsuspend, /* 72 */ \
            sys_sigpending, /* 73 */ \
            sys_sethostname, /* 74 */ \
            sys_setrlimit, /* 75 */ \
            NULL, /* 76 */ \
            sys_getrusage, /* 77 */ \
            sys_gettimeofday, /* 78 */ \
            sys_settimeofday, /* 79 */ \
            sys_getgroups, /* 80 */ \
            sys_setgroups, /* 81 */ \
            NULL, /* 82 */ \
            sys_symlink, /* 83 */ \
            NULL, /* 84 */ \
            sys_readlink, /* 85 */ \
            sys_uselib, /* 86 */ \
            sys_swapon, /* 87 */ \
            sys_reboot, /* 88 */ \
            NULL, /* 89 */ \
            NULL, /* 90 */ \
            sys_munmap, /* 91 */ \
            sys_truncate, /* 92 */ \
            sys_ftruncate, /* 93 */ \
            sys_fchmod, /* 94 */ \
            sys_fchown, /* 95 */ \
            sys_getpriority, /* 96 */ \
            sys_setpriority, /* 97 */ \
            NULL, /* 98 */ \
            sys_statfs, /* 99 */ \
            sys_fstatfs, /* 100 */ \
            NULL, /* 101 */ \
            NULL, /* 102 */ \
            sys_syslog, /* 103 */ \
            sys_setitimer, /* 104 */ \
            sys_getitimer, /* 105 */ \
            sys_stat, /* 106 */ \
            sys_lstat, /* 107 */ \
            sys_fstat, /* 108 */ \
            NULL, /* 109 */ \
            NULL, /* 110 */ \
            sys_vhangup, /* 111 */ \
            NULL, /* 112 */ \
            NULL, /* 113 */ \
            sys_wait4, /* 114 */ \
            sys_swapoff, /* 115 */ \
            sys_sysinfo, /* 116 */ \
            NULL, /* 117 */ \
            sys_fsync, /* 118 */ \
            sys_sigreturn, /* 119 */ \
            sys_clone, /* 120 */ \
            sys_setdomainname, /* 121 */ \
            sys_uname, /* 122 */ \
            NULL, /* 123 */ \
            sys_adjtimex, /* 124 */ \
            sys_mprotect, /* 125 */ \
            sys_sigprocmask, /* 126 */ \
            NULL, /* 127 */ \
            sys_init_module, /* 128 */ \
            sys_delete_module, /* 129 */ \
            NULL, /* 130 */ \
            sys_quotactl, /* 131 */ \
            sys_getpgid, /* 132 */ \
            sys_fchdir, /* 133 */ \
            sys_bdflush, /* 134 */ \
            sys_sysfs, /* 135 */ \
            sys_personality, /* 136 */ \
            NULL, /* 137 */ \
            sys_setfsuid, /* 138 */ \
            sys_setfsgid, /* 139 */ \
            NULL, /* 140 */ \
            sys_getdents, /* 141 */ \
            sys__newselect, /* 142 */ \
            sys_flock, /* 143 */ \
            sys_msync, /* 144 */ \
            sys_readv, /* 145 */ \
            sys_writev, /* 146 */ \
            sys_getsid, /* 147 */ \
            sys_fdatasync, /* 148 */ \
            sys__sysctl, /* 149 */ \
            sys_mlock, /* 150 */ \
            sys_munlock, /* 151 */ \
            sys_mlockall, /* 152 */ \
            sys_munlockall, /* 153 */ \
            sys_sched_setparam, /* 154 */ \
            sys_sched_getparam, /* 155 */ \
            sys_sched_setscheduler, /* 156 */ \
            sys_sched_getscheduler, /* 157 */ \
            sys_sched_yield, /* 158 */ \
            sys_sched_get_priority_max, /* 159 */ \
            sys_sched_get_priority_min, /* 160 */ \
            sys_sched_rr_get_interval, /* 161 */ \
            sys_nanosleep, /* 162 */ \
            sys_mremap, /* 163 */ \
            sys_setresuid, /* 164 */ \
            sys_getresuid, /* 165 */ \
            NULL, /* 166 */ \
            NULL, /* 167 */ \
            sys_poll, /* 168 */ \
            sys_nfsservctl, /* 169 */ \
            sys_setresgid, /* 170 */ \
            sys_getresgid, /* 171 */ \
            sys_prctl, /* 172 */ \
            sys_rt_sigreturn, /* 173 */ \
            sys_rt_sigaction, /* 174 */ \
            sys_rt_sigprocmask, /* 175 */ \
            sys_rt_sigpending, /* 176 */ \
            sys_rt_sigtimedwait, /* 177 */ \
            sys_rt_sigqueueinfo, /* 178 */ \
            sys_rt_sigsuspend, /* 179 */ \
            sys_pread64, /* 180 */ \
            sys_pwrite64, /* 181 */ \
            sys_chown, /* 182 */ \
            sys_getcwd, /* 183 */ \
            sys_capget, /* 184 */ \
            sys_capset, /* 185 */ \
            sys_sigaltstack, /* 186 */ \
            sys_sendfile, /* 187 */ \
            NULL, /* 188 */ \
            NULL, /* 189 */ \
            sys_vfork, /* 190 */ \
            sys_ugetrlimit, /* 191 */ \
            sys_mmap2, /* 192 */ \
            sys_truncate64, /* 193 */ \
            sys_ftruncate64, /* 194 */ \
            sys_stat64, /* 195 */ \
            sys_lstat64, /* 196 */ \
            sys_fstat64, /* 197 */ \
            sys_lchown32, /* 198 */ \
            sys_getuid32, /* 199 */ \
            sys_getgid32, /* 200 */ \
            sys_geteuid32, /* 201 */ \
            sys_getegid32, /* 202 */ \
            sys_setreuid32, /* 203 */ \
            sys_setregid32, /* 204 */ \
            sys_getgroups32, /* 205 */ \
            sys_setgroups32, /* 206 */ \
            sys_fchown32, /* 207 */ \
            sys_setresuid32, /* 208 */ \
            sys_getresuid32, /* 209 */ \
            sys_setresgid32, /* 210 */ \
            sys_getresgid32, /* 211 */ \
            sys_chown32, /* 212 */ \
            sys_setuid32, /* 213 */ \
            sys_setgid32, /* 214 */ \
            sys_setfsuid32, /* 215 */ \
            sys_setfsgid32, /* 216 */ \
            sys_getdents64, /* 217 */ \
            sys_pivot_root, /* 218 */ \
            sys_mincore, /* 219 */ \
            sys_madvise, /* 220 */ \
            sys_fcntl64, /* 221 */ \
            NULL, /* 222 */ \
            NULL, /* 223 */ \
            sys_gettid, /* 224 */ \
            sys_readahead, /* 225 */ \
            sys_setxattr, /* 226 */ \
            sys_lsetxattr, /* 227 */ \
            sys_fsetxattr, /* 228 */ \
            sys_getxattr, /* 229 */ \
            sys_lgetxattr, /* 230 */ \
            sys_fgetxattr, /* 231 */ \
            sys_listxattr, /* 232 */ \
            sys_llistxattr, /* 233 */ \
            sys_flistxattr, /* 234 */ \
            sys_removexattr, /* 235 */ \
            sys_lremovexattr, /* 236 */ \
            sys_fremovexattr, /* 237 */ \
            sys_tkill, /* 238 */ \
            sys_sendfile64, /* 239 */ \
            sys_futex, /* 240 */ \
            sys_sched_setaffinity, /* 241 */ \
            sys_sched_getaffinity, /* 242 */ \
            sys_io_setup, /* 243 */ \
            sys_io_destroy, /* 244 */ \
            sys_io_getevents, /* 245 */ \
            sys_io_submit, /* 246 */ \
            sys_io_cancel, /* 247 */ \
            sys_exit_group, /* 248 */ \
            sys_lookup_dcookie, /* 249 */ \
            sys_epoll_create, /* 250 */ \
            sys_epoll_ctl, /* 251 */ \
            sys_epoll_wait, /* 252 */ \
            sys_remap_file_pages, /* 253 */ \
            NULL, /* 254 */ \
            NULL, /* 255 */ \
            sys_set_tid_address, /* 256 */ \
            sys_timer_create, /* 257 */ \
            sys_timer_settime, /* 258 */ \
            sys_timer_gettime, /* 259 */ \
            sys_timer_getoverrun, /* 260 */ \
            sys_timer_delete, /* 261 */ \
            sys_clock_settime, /* 262 */ \
            sys_clock_gettime, /* 263 */ \
            sys_clock_getres, /* 264 */ \
            sys_clock_nanosleep, /* 265 */ \
            sys_statfs64, /* 266 */ \
            sys_fstatfs64, /* 267 */ \
            sys_tgkill, /* 268 */ \
            sys_utimes, /* 269 */ \
            sys_fadvise64_64, /* 270 */ \
            sys_pciconfig_iobase, /* 271 */ \
            sys_pciconfig_read, /* 272 */ \
            sys_pciconfig_write, /* 273 */ \
            sys_mq_open, /* 274 */ \
            sys_mq_unlink, /* 275 */ \
            sys_mq_timedsend, /* 276 */ \
            sys_mq_timedreceive, /* 277 */ \
            sys_mq_notify, /* 278 */ \
            sys_mq_getsetattr, /* 279 */ \
            sys_waitid, /* 280 */ \
            sys_socket, /* 281 */ \
            sys_bind, /* 282 */ \
            sys_connect, /* 283 */ \
            sys_listen, /* 284 */ \
            sys_accept, /* 285 */ \
            sys_getsockname, /* 286 */ \
            sys_getpeername, /* 287 */ \
            sys_socketpair, /* 288 */ \
            sys_send, /* 289 */ \
            sys_sendto, /* 290 */ \
            sys_recv, /* 291 */ \
            sys_recvfrom, /* 292 */ \
            sys_shutdown, /* 293 */ \
            sys_setsockopt, /* 294 */ \
            sys_getsockopt, /* 295 */ \
            sys_sendmsg, /* 296 */ \
            sys_recvmsg, /* 297 */ \
            sys_semop, /* 298 */ \
            sys_semget, /* 299 */ \
            sys_semctl, /* 300 */ \
            sys_msgsnd, /* 301 */ \
            sys_msgrcv, /* 302 */ \
            sys_msgget, /* 303 */ \
            sys_msgctl, /* 304 */ \
            sys_shmat, /* 305 */ \
            sys_shmdt, /* 306 */ \
            sys_shmget, /* 307 */ \
            sys_shmctl, /* 308 */ \
            sys_add_key, /* 309 */ \
            sys_request_key, /* 310 */ \
            sys_keyctl, /* 311 */ \
            sys_semtimedop, /* 312 */ \
            sys_vserver, /* 313 */ \
            sys_ioprio_set, /* 314 */ \
            sys_ioprio_get, /* 315 */ \
            sys_inotify_init, /* 316 */ \
            sys_inotify_add_watch, /* 317 */ \
            sys_inotify_rm_watch, /* 318 */ \
            sys_mbind, /* 319 */ \
            sys_get_mempolicy, /* 320 */ \
            sys_set_mempolicy, /* 321 */ \
            sys_openat, /* 322 */ \
            sys_mkdirat, /* 323 */ \
            sys_mknodat, /* 324 */ \
            sys_fchownat, /* 325 */ \
            sys_futimesat, /* 326 */ \
            sys_fstatat64, /* 327 */ \
            sys_unlinkat, /* 328 */ \
            sys_renameat, /* 329 */ \
            sys_linkat, /* 330 */ \
            sys_symlinkat, /* 331 */ \
            sys_readlinkat, /* 332 */ \
            sys_fchmodat, /* 333 */ \
            sys_faccessat, /* 334 */ \
            sys_pselect6, /* 335 */ \
            sys_ppoll, /* 336 */ \
            sys_unshare, /* 337 */ \
            sys_set_robust_list, /* 338 */ \
            sys_get_robust_list, /* 339 */ \
            sys_splice, /* 340 */ \
            sys_sync_file_range2, /* 341 */ \
            sys_tee, /* 342 */ \
            sys_vmsplice, /* 343 */ \
            sys_move_pages, /* 344 */ \
            sys_getcpu, /* 345 */ \
            sys_epoll_pwait, /* 346 */ \
            sys_kexec_load, /* 347 */ \
            sys_utimensat, /* 348 */ \
            sys_signalfd, /* 349 */ \
            sys_timerfd_create, /* 350 */ \
            sys_eventfd, /* 351 */ \
            sys_fallocate, /* 352 */ \
            sys_timerfd_settime, /* 353 */ \
            sys_timerfd_gettime, /* 354 */ \
            sys_signalfd4, /* 355 */ \
            sys_eventfd2, /* 356 */ \
            sys_epoll_create1, /* 357 */ \
            sys_dup3, /* 358 */ \
            sys_pipe2, /* 359 */ \
            sys_inotify_init1, /* 360 */ \
            sys_preadv, /* 361 */ \
            sys_pwritev, /* 362 */ \
            sys_rt_tgsigqueueinfo, /* 363 */ \
            sys_perf_event_open, /* 364 */ \
            sys_recvmmsg, /* 365 */ \
            sys_accept4, /* 366 */ \
            sys_fanotify_init, /* 367 */ \
            sys_fanotify_mark, /* 368 */ \
            sys_prlimit64, /* 369 */ \
            sys_name_to_handle_at, /* 370 */ \
            sys_open_by_handle_at, /* 371 */ \
            sys_clock_adjtime, /* 372 */ \
            sys_syncfs, /* 373 */ \
            sys_sendmmsg, /* 374 */ \
            sys_setns, /* 375 */ \
            sys_process_vm_readv, /* 376 */ \
            sys_process_vm_writev, /* 377 */ \
        }; \
        __muslc_syscall_ptr_table = &__muslc_syscall_ptr_table_impl; \
    } while (0)

/* Previously the syscall table and setup for it were separate macros. This
 * legacy macro is left for projects still expecting it to expand to the
 * syscall table.
 */
#define MUSLC_SYSCALL_TABLE /* nothing */

#endif /* _SYSCALL_STUBS_SEL4_ARM_H_ */
