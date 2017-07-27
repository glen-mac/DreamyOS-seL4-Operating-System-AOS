/* @LICENSE(NICTA) */

#ifndef _SYSCALL_STUBS_SEL4_IA32_H_
#define _SYSCALL_STUBS_SEL4_IA32_H_

#include <stdarg.h>
#include <syscall_sel4.h>

long sys_restart_syscall(va_list ap);
long sys_exit(va_list ap);
long sys_fork(va_list ap);
long sys_read(va_list ap);
long sys_write(va_list ap);
long sys_open(va_list ap);
long sys_close(va_list ap);
long sys_waitpid(va_list ap);
long sys_creat(va_list ap);
long sys_link(va_list ap);
long sys_unlink(va_list ap);
long sys_execve(va_list ap);
long sys_chdir(va_list ap);
long sys_time(va_list ap);
long sys_mknod(va_list ap);
long sys_chmod(va_list ap);
long sys_lchown(va_list ap);
long sys_break(va_list ap);
long sys_oldstat(va_list ap);
long sys_lseek(va_list ap);
long sys_getpid(va_list ap);
long sys_mount(va_list ap);
long sys_umount(va_list ap);
long sys_setuid(va_list ap);
long sys_getuid(va_list ap);
long sys_stime(va_list ap);
long sys_ptrace(va_list ap);
long sys_alarm(va_list ap);
long sys_oldfstat(va_list ap);
long sys_pause(va_list ap);
long sys_utime(va_list ap);
long sys_stty(va_list ap);
long sys_gtty(va_list ap);
long sys_access(va_list ap);
long sys_nice(va_list ap);
long sys_ftime(va_list ap);
long sys_sync(va_list ap);
long sys_kill(va_list ap);
long sys_rename(va_list ap);
long sys_mkdir(va_list ap);
long sys_rmdir(va_list ap);
long sys_dup(va_list ap);
long sys_pipe(va_list ap);
long sys_times(va_list ap);
long sys_prof(va_list ap);
long sys_brk(va_list ap);
long sys_setgid(va_list ap);
long sys_getgid(va_list ap);
long sys_signal(va_list ap);
long sys_geteuid(va_list ap);
long sys_getegid(va_list ap);
long sys_acct(va_list ap);
long sys_umount2(va_list ap);
long sys_lock(va_list ap);
long sys_ioctl(va_list ap);
long sys_fcntl(va_list ap);
long sys_mpx(va_list ap);
long sys_setpgid(va_list ap);
long sys_ulimit(va_list ap);
long sys_oldolduname(va_list ap);
long sys_umask(va_list ap);
long sys_chroot(va_list ap);
long sys_ustat(va_list ap);
long sys_dup2(va_list ap);
long sys_getppid(va_list ap);
long sys_getpgrp(va_list ap);
long sys_setsid(va_list ap);
long sys_sigaction(va_list ap);
long sys_sgetmask(va_list ap);
long sys_ssetmask(va_list ap);
long sys_setreuid(va_list ap);
long sys_setregid(va_list ap);
long sys_sigsuspend(va_list ap);
long sys_sigpending(va_list ap);
long sys_sethostname(va_list ap);
long sys_setrlimit(va_list ap);
long sys_getrlimit(va_list ap);
long sys_getrusage(va_list ap);
long sys_gettimeofday(va_list ap);
long sys_settimeofday(va_list ap);
long sys_getgroups(va_list ap);
long sys_setgroups(va_list ap);
long sys_select(va_list ap);
long sys_symlink(va_list ap);
long sys_oldlstat(va_list ap);
long sys_readlink(va_list ap);
long sys_uselib(va_list ap);
long sys_swapon(va_list ap);
long sys_reboot(va_list ap);
long sys_readdir(va_list ap);
long sys_mmap(va_list ap);
long sys_munmap(va_list ap);
long sys_truncate(va_list ap);
long sys_ftruncate(va_list ap);
long sys_fchmod(va_list ap);
long sys_fchown(va_list ap);
long sys_getpriority(va_list ap);
long sys_setpriority(va_list ap);
long sys_profil(va_list ap);
long sys_statfs(va_list ap);
long sys_fstatfs(va_list ap);
long sys_ioperm(va_list ap);
long sys_socketcall(va_list ap);
long sys_syslog(va_list ap);
long sys_setitimer(va_list ap);
long sys_getitimer(va_list ap);
long sys_stat(va_list ap);
long sys_lstat(va_list ap);
long sys_fstat(va_list ap);
long sys_olduname(va_list ap);
long sys_iopl(va_list ap);
long sys_vhangup(va_list ap);
long sys_idle(va_list ap);
long sys_vm86old(va_list ap);
long sys_wait4(va_list ap);
long sys_swapoff(va_list ap);
long sys_sysinfo(va_list ap);
long sys_ipc(va_list ap);
long sys_fsync(va_list ap);
long sys_clone(va_list ap);
long sys_setdomainname(va_list ap);
long sys_uname(va_list ap);
long sys_modify_ldt(va_list ap);
long sys_adjtimex(va_list ap);
long sys_mprotect(va_list ap);
long sys_sigprocmask(va_list ap);
long sys_create_module(va_list ap);
long sys_init_module(va_list ap);
long sys_delete_module(va_list ap);
long sys_get_kernel_syms(va_list ap);
long sys_quotactl(va_list ap);
long sys_getpgid(va_list ap);
long sys_fchdir(va_list ap);
long sys_bdflush(va_list ap);
long sys_sysfs(va_list ap);
long sys_personality(va_list ap);
long sys_afs_syscall(va_list ap);
long sys_setfsuid(va_list ap);
long sys_setfsgid(va_list ap);
long sys__llseek(va_list ap);
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
long sys_vm86(va_list ap);
long sys_query_module(va_list ap);
long sys_poll(va_list ap);
long sys_nfsservctl(va_list ap);
long sys_setresgid(va_list ap);
long sys_getresgid(va_list ap);
long sys_prctl(va_list ap);
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
long sys_getpmsg(va_list ap);
long sys_putpmsg(va_list ap);
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
long sys_pivot_root(va_list ap);
long sys_mincore(va_list ap);
long sys_madvise(va_list ap);
long sys_madvise1(va_list ap);
long sys_getdents64(va_list ap);
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
long sys_set_thread_area(va_list ap);
long sys_get_thread_area(va_list ap);
long sys_io_setup(va_list ap);
long sys_io_destroy(va_list ap);
long sys_io_getevents(va_list ap);
long sys_io_submit(va_list ap);
long sys_io_cancel(va_list ap);
long sys_fadvise64(va_list ap);
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
long sys_vserver(va_list ap);
long sys_mbind(va_list ap);
long sys_get_mempolicy(va_list ap);
long sys_set_mempolicy(va_list ap);
long sys_mq_open(va_list ap);
long sys_mq_unlink(va_list ap);
long sys_mq_timedsend(va_list ap);
long sys_mq_timedreceive(va_list ap);
long sys_mq_notify(va_list ap);
long sys_mq_getsetattr(va_list ap);
long sys_kexec_load(va_list ap);
long sys_waitid(va_list ap);
long sys_add_key(va_list ap);
long sys_request_key(va_list ap);
long sys_keyctl(va_list ap);
long sys_ioprio_set(va_list ap);
long sys_ioprio_get(va_list ap);
long sys_inotify_init(va_list ap);
long sys_inotify_add_watch(va_list ap);
long sys_inotify_rm_watch(va_list ap);
long sys_migrate_pages(va_list ap);
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
long sys_sync_file_range(va_list ap);
long sys_tee(va_list ap);
long sys_vmsplice(va_list ap);
long sys_move_pages(va_list ap);
long sys_getcpu(va_list ap);
long sys_epoll_pwait(va_list ap);
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
long sys_prlimit64(va_list ap);
long sys_name_to_handle_at(va_list ap);
long sys_open_by_handle_at(va_list ap);
long sys_clock_adjtime(va_list ap);
long sys_syncfs(va_list ap);
long sys_sendmmsg(va_list ap);
long sys_setns(va_list ap);
long sys_process_vm_readv(va_list ap);
long sys_process_vm_writev(va_list ap);
long sys_fstatat(va_list ap);
long sys_pread(va_list ap);
long sys_pwrite(va_list ap);
long sys_fadvise(va_list ap);
long sys_sigreturn(va_list ap);
long sys_rt_sigreturn(va_list ap);

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
            sys_waitpid, /* 7 */ \
            sys_creat, /* 8 */ \
            sys_link, /* 9 */ \
            sys_unlink, /* 10 */ \
            sys_execve, /* 11 */ \
            sys_chdir, /* 12 */ \
            sys_time, /* 13 */ \
            sys_mknod, /* 14 */ \
            sys_chmod, /* 15 */ \
            sys_lchown, /* 16 */ \
            sys_break, /* 17 */ \
            sys_oldstat, /* 18 */ \
            sys_lseek, /* 19 */ \
            sys_getpid, /* 20 */ \
            sys_mount, /* 21 */ \
            sys_umount, /* 22 */ \
            sys_setuid, /* 23 */ \
            sys_getuid, /* 24 */ \
            sys_stime, /* 25 */ \
            sys_ptrace, /* 26 */ \
            sys_alarm, /* 27 */ \
            sys_oldfstat, /* 28 */ \
            sys_pause, /* 29 */ \
            sys_utime, /* 30 */ \
            sys_stty, /* 31 */ \
            sys_gtty, /* 32 */ \
            sys_access, /* 33 */ \
            sys_nice, /* 34 */ \
            sys_ftime, /* 35 */ \
            sys_sync, /* 36 */ \
            sys_kill, /* 37 */ \
            sys_rename, /* 38 */ \
            sys_mkdir, /* 39 */ \
            sys_rmdir, /* 40 */ \
            sys_dup, /* 41 */ \
            sys_pipe, /* 42 */ \
            sys_times, /* 43 */ \
            sys_prof, /* 44 */ \
            sys_brk, /* 45 */ \
            sys_setgid, /* 46 */ \
            sys_getgid, /* 47 */ \
            sys_signal, /* 48 */ \
            sys_geteuid, /* 49 */ \
            sys_getegid, /* 50 */ \
            sys_acct, /* 51 */ \
            sys_umount2, /* 52 */ \
            sys_lock, /* 53 */ \
            sys_ioctl, /* 54 */ \
            sys_fcntl, /* 55 */ \
            sys_mpx, /* 56 */ \
            sys_setpgid, /* 57 */ \
            sys_ulimit, /* 58 */ \
            sys_oldolduname, /* 59 */ \
            sys_umask, /* 60 */ \
            sys_chroot, /* 61 */ \
            sys_ustat, /* 62 */ \
            sys_dup2, /* 63 */ \
            sys_getppid, /* 64 */ \
            sys_getpgrp, /* 65 */ \
            sys_setsid, /* 66 */ \
            sys_sigaction, /* 67 */ \
            sys_sgetmask, /* 68 */ \
            sys_ssetmask, /* 69 */ \
            sys_setreuid, /* 70 */ \
            sys_setregid, /* 71 */ \
            sys_sigsuspend, /* 72 */ \
            sys_sigpending, /* 73 */ \
            sys_sethostname, /* 74 */ \
            sys_setrlimit, /* 75 */ \
            sys_getrlimit, /* 76 */ \
            sys_getrusage, /* 77 */ \
            sys_gettimeofday, /* 78 */ \
            sys_settimeofday, /* 79 */ \
            sys_getgroups, /* 80 */ \
            sys_setgroups, /* 81 */ \
            sys_select, /* 82 */ \
            sys_symlink, /* 83 */ \
            sys_oldlstat, /* 84 */ \
            sys_readlink, /* 85 */ \
            sys_uselib, /* 86 */ \
            sys_swapon, /* 87 */ \
            sys_reboot, /* 88 */ \
            sys_readdir, /* 89 */ \
            sys_mmap, /* 90 */ \
            sys_munmap, /* 91 */ \
            sys_truncate, /* 92 */ \
            sys_ftruncate, /* 93 */ \
            sys_fchmod, /* 94 */ \
            sys_fchown, /* 95 */ \
            sys_getpriority, /* 96 */ \
            sys_setpriority, /* 97 */ \
            sys_profil, /* 98 */ \
            sys_statfs, /* 99 */ \
            sys_fstatfs, /* 100 */ \
            sys_ioperm, /* 101 */ \
            sys_socketcall, /* 102 */ \
            sys_syslog, /* 103 */ \
            sys_setitimer, /* 104 */ \
            sys_getitimer, /* 105 */ \
            sys_stat, /* 106 */ \
            sys_lstat, /* 107 */ \
            sys_fstat, /* 108 */ \
            sys_olduname, /* 109 */ \
            sys_iopl, /* 110 */ \
            sys_vhangup, /* 111 */ \
            sys_idle, /* 112 */ \
            sys_vm86old, /* 113 */ \
            sys_wait4, /* 114 */ \
            sys_swapoff, /* 115 */ \
            sys_sysinfo, /* 116 */ \
            sys_ipc, /* 117 */ \
            sys_fsync, /* 118 */ \
            sys_sigreturn, /* 119 */ \
            sys_clone, /* 120 */ \
            sys_setdomainname, /* 121 */ \
            sys_uname, /* 122 */ \
            sys_modify_ldt, /* 123 */ \
            sys_adjtimex, /* 124 */ \
            sys_mprotect, /* 125 */ \
            sys_sigprocmask, /* 126 */ \
            sys_create_module, /* 127 */ \
            sys_init_module, /* 128 */ \
            sys_delete_module, /* 129 */ \
            sys_get_kernel_syms, /* 130 */ \
            sys_quotactl, /* 131 */ \
            sys_getpgid, /* 132 */ \
            sys_fchdir, /* 133 */ \
            sys_bdflush, /* 134 */ \
            sys_sysfs, /* 135 */ \
            sys_personality, /* 136 */ \
            sys_afs_syscall, /* 137 */ \
            sys_setfsuid, /* 138 */ \
            sys_setfsgid, /* 139 */ \
            sys__llseek, /* 140 */ \
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
            sys_vm86, /* 166 */ \
            sys_query_module, /* 167 */ \
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
            sys_getpmsg, /* 188 */ \
            sys_putpmsg, /* 189 */ \
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
            sys_pivot_root, /* 217 */ \
            sys_mincore, /* 218 */ \
            sys_madvise, /* 219 */ \
            sys_getdents64, /* 220 */ \
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
            sys_set_thread_area, /* 243 */ \
            sys_get_thread_area, /* 244 */ \
            sys_io_setup, /* 245 */ \
            sys_io_destroy, /* 246 */ \
            sys_io_getevents, /* 247 */ \
            sys_io_submit, /* 248 */ \
            sys_io_cancel, /* 249 */ \
            sys_fadvise64, /* 250 */ \
            NULL, /* 251 */ \
            sys_exit_group, /* 252 */ \
            sys_lookup_dcookie, /* 253 */ \
            sys_epoll_create, /* 254 */ \
            sys_epoll_ctl, /* 255 */ \
            sys_epoll_wait, /* 256 */ \
            sys_remap_file_pages, /* 257 */ \
            sys_set_tid_address, /* 258 */ \
            sys_timer_create, /* 259 */ \
            sys_timer_settime, /* 260 */ \
            sys_timer_gettime, /* 261 */ \
            sys_timer_getoverrun, /* 262 */ \
            sys_timer_delete, /* 263 */ \
            sys_clock_settime, /* 264 */ \
            sys_clock_gettime, /* 265 */ \
            sys_clock_getres, /* 266 */ \
            sys_clock_nanosleep, /* 267 */ \
            sys_statfs64, /* 268 */ \
            sys_fstatfs64, /* 269 */ \
            sys_tgkill, /* 270 */ \
            sys_utimes, /* 271 */ \
            sys_fadvise64_64, /* 272 */ \
            sys_vserver, /* 273 */ \
            sys_mbind, /* 274 */ \
            sys_get_mempolicy, /* 275 */ \
            sys_set_mempolicy, /* 276 */ \
            sys_mq_open, /* 277 */ \
            sys_mq_unlink, /* 278 */ \
            sys_mq_timedsend, /* 279 */ \
            sys_mq_timedreceive, /* 280 */ \
            sys_mq_notify, /* 281 */ \
            sys_mq_getsetattr, /* 282 */ \
            sys_kexec_load, /* 283 */ \
            sys_waitid, /* 284 */ \
            NULL, /* 285 */ \
            sys_add_key, /* 286 */ \
            sys_request_key, /* 287 */ \
            sys_keyctl, /* 288 */ \
            sys_ioprio_set, /* 289 */ \
            sys_ioprio_get, /* 290 */ \
            sys_inotify_init, /* 291 */ \
            sys_inotify_add_watch, /* 292 */ \
            sys_inotify_rm_watch, /* 293 */ \
            sys_migrate_pages, /* 294 */ \
            sys_openat, /* 295 */ \
            sys_mkdirat, /* 296 */ \
            sys_mknodat, /* 297 */ \
            sys_fchownat, /* 298 */ \
            sys_futimesat, /* 299 */ \
            sys_fstatat64, /* 300 */ \
            sys_unlinkat, /* 301 */ \
            sys_renameat, /* 302 */ \
            sys_linkat, /* 303 */ \
            sys_symlinkat, /* 304 */ \
            sys_readlinkat, /* 305 */ \
            sys_fchmodat, /* 306 */ \
            sys_faccessat, /* 307 */ \
            sys_pselect6, /* 308 */ \
            sys_ppoll, /* 309 */ \
            sys_unshare, /* 310 */ \
            sys_set_robust_list, /* 311 */ \
            sys_get_robust_list, /* 312 */ \
            sys_splice, /* 313 */ \
            sys_sync_file_range, /* 314 */ \
            sys_tee, /* 315 */ \
            sys_vmsplice, /* 316 */ \
            sys_move_pages, /* 317 */ \
            sys_getcpu, /* 318 */ \
            sys_epoll_pwait, /* 319 */ \
            sys_utimensat, /* 320 */ \
            sys_signalfd, /* 321 */ \
            sys_timerfd_create, /* 322 */ \
            sys_eventfd, /* 323 */ \
            sys_fallocate, /* 324 */ \
            sys_timerfd_settime, /* 325 */ \
            sys_timerfd_gettime, /* 326 */ \
            sys_signalfd4, /* 327 */ \
            sys_eventfd2, /* 328 */ \
            sys_epoll_create1, /* 329 */ \
            sys_dup3, /* 330 */ \
            sys_pipe2, /* 331 */ \
            sys_inotify_init1, /* 332 */ \
            sys_preadv, /* 333 */ \
            sys_pwritev, /* 334 */ \
            NULL, /* 335 */ \
            NULL, /* 336 */ \
            NULL, /* 337 */ \
            NULL, /* 338 */ \
            NULL, /* 339 */ \
            sys_prlimit64, /* 340 */ \
            sys_name_to_handle_at, /* 341 */ \
            sys_open_by_handle_at, /* 342 */ \
            sys_clock_adjtime, /* 343 */ \
            sys_syncfs, /* 344 */ \
            sys_sendmmsg, /* 345 */ \
            sys_setns, /* 346 */ \
            sys_process_vm_readv, /* 347 */ \
            sys_process_vm_writev, /* 348 */ \
        /* Padding so ia32 syscall table is same length as arm */ \
            NULL, /* 349 */ \
            NULL, /* 350 */ \
            NULL, /* 351 */ \
            NULL, /* 352 */ \
            NULL, /* 353 */ \
            NULL, /* 354 */ \
            NULL, /* 355 */ \
            NULL, /* 356 */ \
            NULL, /* 357 */ \
            NULL, /* 358 */ \
            NULL, /* 359 */ \
            NULL, /* 360 */ \
            NULL, /* 361 */ \
            NULL, /* 362 */ \
            NULL, /* 363 */ \
            NULL, /* 364 */ \
            NULL, /* 365 */ \
            NULL, /* 366 */ \
            NULL, /* 367 */ \
            NULL, /* 368 */ \
            NULL, /* 369 */ \
            NULL, /* 370 */ \
            NULL, /* 371 */ \
            NULL, /* 372 */ \
            NULL, /* 373 */ \
            NULL, /* 374 */ \
            NULL, /* 375 */ \
            NULL, /* 376 */ \
            NULL, /* 377 */ \
        }; \
        __muslc_syscall_ptr_table = &__muslc_syscall_ptr_table_impl; \
    } while (0)

/* Previously the syscall table and setup for it were separate macros. This
 * legacy macro is left for projects still expecting it to expand to the
 * syscall table.
 */
#define MUSLC_SYSCALL_TABLE /* nothing */

#endif /* _SYSCALL_STUBS_SEL4_IA32_H_ */
