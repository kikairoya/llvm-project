//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is shared between various sanitizers' runtime libraries and
// implements Cygwin-specific functions.
//===----------------------------------------------------------------------===//

#include "sanitizer_platform.h"

#if SANITIZER_CYGWIN

#  include <dlfcn.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <limits.h>
#  include <memoryapi.h>
#  include <process.h>
#  include <pthread.h>
#  include <sched.h>
#  include <signal.h>
#  include <spawn.h>
#  include <sys/cygwin.h>
#  include <sys/mman.h>
#  include <sys/param.h>
#  include <sys/resource.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <time.h>
#  include <ucontext.h>
#  include <unistd.h>
#  include <windows.h>

#  include "interception/interception.h"
#  include "sanitizer_common.h"
#  include "sanitizer_file.h"
#  include "sanitizer_flags.h"
#  include "sanitizer_getauxval.h"
#  include "sanitizer_internal_defs.h"
#  include "sanitizer_libc.h"
#  include "sanitizer_linux.h"
#  include "sanitizer_mutex.h"
#  include "sanitizer_placement_new.h"
#  include "sanitizer_procmaps.h"

DECLARE_REAL(void*, mmap, void*, SIZE_T, int, int, int, off_t);
DECLARE_REAL(int, munmap, void* a, SIZE_T b);
DECLARE_REAL(int, mprotect, void* a, SIZE_T b, int c);
DECLARE_REAL(int, read, int, void*, SIZE_T);
DECLARE_REAL(int, write, int, const void*, SIZE_T);

namespace __sanitizer {

static FARPROC GetRealLibcAddress(const char* symbol) {
  HMODULE cygwin1;
  // if (!GetModuleHandleExW(
  //       GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
  //       GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCWSTR)&cygwin_internal, &cygwin1))
  //       {
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"cygwin1.dll",
                          &cygwin1)) {
    // Printf("GetRealLibcAddress failed for symbol=%s", symbol);
    Die();
  }
  auto real = GetProcAddress(cygwin1, symbol);
  if (!real) {
    // Printf("GetRealLibcAddress failed for symbol=%s", symbol);
    Die();
  }
  return real;
}

#  define _REAL(func, ...) real##_##func(__VA_ARGS__)
#  define DEFINE__REAL(ret_type, func, ...)                               \
    static ret_type (*real_##func)(__VA_ARGS__) = NULL;                   \
    if (!real_##func) {                                                   \
      real_##func = (ret_type (*)(__VA_ARGS__))GetRealLibcAddress(#func); \
    }                                                                     \
    CHECK(real_##func);
#  define CALL_REAL(func, ...) \
    (REAL(func) ? REAL(func)(__VA_ARGS__) : func(__VA_ARGS__))
// #define CALL_REAL(func, ...) (func(__VA_ARGS__))

// --------------- sanitizer_libc.h
uptr internal_mmap(void* addr, uptr length, int prot, int flags, int fd,
                   u64 offset) {
  // DEFINE__REAL(uptr, mmap, void *, uptr, int, int, int, u64);
  // uptr ret = _REAL(mmap, addr, length, prot, flags, fd, offset);
  uptr ret = (uptr)CALL_REAL(mmap, addr, length, prot, flags, fd, offset);
  if (ret == (uptr)-1 && addr &&
      (flags & (MAP_NORESERVE | MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS)) ==
          (MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS) &&
      internal_mprotect(addr, length, prot) == 0) {
    // if ((prot & PROT_WRITE) == PROT_WRITE)
    //   internal_memset(addr, 0, length);
    return (uptr)addr;
  }
  if (ret != (uptr)-1 && ret && !addr &&
      ((flags & (MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS)) ==
       (MAP_PRIVATE | MAP_ANONYMOUS)) &&
      ((prot & (PROT_READ | PROT_WRITE)) == (PROT_READ | PROT_WRITE))) {
    internal_memset((void*)ret, 0, length);
  }
  return ret;
}

uptr internal_munmap(void* addr, uptr length) {
  // DEFINE__REAL(int, munmap, void *a, uptr b);
  return CALL_REAL(munmap, addr, length);
}

uptr internal_mremap(void* old_address, uptr old_size, uptr new_size, int flags,
                     void* new_address) {
  CHECK(false && "internal_mremap is unimplemented on NetBSD");
  return 0;
}

int internal_mprotect(void* addr, uptr length, int prot) {
  return CALL_REAL(mprotect, addr, length, prot);
}

int internal_madvise(uptr addr, uptr length, int advice) {
  DEFINE__REAL(int, madvise, void* a, uptr b, int c);
  return _REAL(madvise, (void*)addr, length, advice);
}

uptr internal_close(fd_t fd) {
  DEFINE__REAL(int, close, int);
  return _REAL(close, fd);
}

uptr internal_close_range(fd_t lowfd, fd_t highfd, int flags) {
  DEFINE__REAL(int, close_range, int, int, int);
  return _REAL(close_range, lowfd, highfd, flags);
}

uptr internal_open(const char* filename, int flags) {
  return internal_open(filename, flags, 0);
}

uptr internal_open(const char* filename, int flags, u32 mode) {
  DEFINE__REAL(int, open, const char*, int, ...);
  return _REAL(open, filename, flags, mode);
}

uptr internal_read(fd_t fd, void* buf, uptr count) {
  sptr res;
  HANDLE_EINTR(res, CALL_REAL(read, fd, buf, (size_t)count));
  return res;
}

uptr internal_write(fd_t fd, const void* buf, uptr count) {
  sptr res;
  HANDLE_EINTR(res, CALL_REAL(write, fd, buf, count));
  return res;
}

uptr internal_ftruncate(fd_t fd, uptr size) {
  DEFINE__REAL(uptr, ftruncate, int, uptr);
  sptr res;
  HANDLE_EINTR(res, _REAL(ftruncate, fd, size));
  return res;
}

uptr internal_stat(const char* path, void* buf) {
  DEFINE__REAL(int, stat, const char* a, void* b);
  return _REAL(stat, path, buf);
}

uptr internal_lstat(const char* path, void* buf) {
  DEFINE__REAL(int, lstat, const char* a, void* b);
  return _REAL(lstat, path, buf);
}

uptr internal_fstat(fd_t fd, void* buf) {
  DEFINE__REAL(int, fstat, int a, void* b);
  return _REAL(fstat, fd, buf);
}

uptr internal_filesize(fd_t fd) {
  struct stat st;
  if (internal_fstat(fd, &st))
    return -1;
  return (uptr)st.st_size;
}

uptr internal_dup(int oldfd) {
  DEFINE__REAL(int, dup, int a);
  return _REAL(dup, oldfd);
}

uptr internal_dup2(int oldfd, int newfd) {
  DEFINE__REAL(int, dup2, int a, int b);
  return _REAL(dup2, oldfd, newfd);
}

uptr internal_readlink(const char* path, char* buf, uptr bufsize) {
  DEFINE__REAL(uptr, readlink, const char*, char*, uptr);
  return (uptr)_REAL(readlink, path, buf, bufsize);
}

uptr internal_unlink(const char* path) {
  DEFINE__REAL(int, unlink, const char* a);
  return _REAL(unlink, path);
}

uptr internal_rename(const char* oldpath, const char* newpath) {
  DEFINE__REAL(int, rename, const char* a, const char* b);
  return _REAL(rename, oldpath, newpath);
}

uptr internal_sched_yield() {
  DEFINE__REAL(int, sched_yield);
  return _REAL(sched_yield);
}

void internal__exit(int exitcode) {
  if (::IsDebuggerPresent())
    __debugbreak();
  cygwin_internal(CW_EXIT_PROCESS, 1);
  Die();  // Unreachable.
}

#  if 0
void internal_usleep(u64 useconds) {
  struct timespec ts;
  ts.tv_sec = useconds / 1000000;
  ts.tv_nsec = (useconds % 1000000) * 1000;
  DEFINE__REAL(int, nanosleep, const struct timespec *, struct timespec *);
  _REAL(nanosleep, &ts, &ts);
}
#  endif

uptr internal_execve(const char* filename, char* const argv[],
                     char* const envp[]) {
  DEFINE__REAL(int, execve, const char*, char* const[], char* const[]);
  return _REAL(execve, filename, argv, envp);
}

int internal_sigaction(int signum, const void* act, void* oldact) {
  DEFINE__REAL(int, sigaction, int, const void*, void*);
  return _REAL(sigaction, signum, act, oldact);
}

#  if 0
ThreadID GetTid() {
  DEFINE__REAL(int, _lwp_self);
  return _REAL(_lwp_self);
}

int TgKill(pid_t pid, ThreadID tid, int sig) {
  DEFINE__REAL(int, _lwp_kill, int a, int b);
  (void)pid;
  return _REAL(_lwp_kill, tid, sig);
}

u64 NanoTime() {
  timeval tv;
  DEFINE__REAL(int, __gettimeofday50, void *a, void *b);
  internal_memset(&tv, 0, sizeof(tv));
  _REAL(__gettimeofday50, &tv, 0);
  return (u64)tv.tv_sec * 1000 * 1000 * 1000 + tv.tv_usec * 1000;
}
#  endif

uptr internal_clock_gettime(__sanitizer_clockid_t clk_id, void* tp) {
  DEFINE__REAL(int, clock_gettime, __sanitizer_clockid_t a, void* b);
  return _REAL(clock_gettime, clk_id, tp);
}

// uptr internal_ptrace(int request, int pid, void *addr, int data) {
//   DEFINE__REAL(int, ptrace, int a, int b, void *c, int d);
//   return _REAL(ptrace, request, pid, addr, data);
// }

uptr internal_waitpid(int pid, int* status, int options) {
  DEFINE__REAL(int, waitpid, int, int*, int);
  return _REAL(waitpid, pid, status, options);
}

uptr internal_getpid() {
  DEFINE__REAL(int, getpid);
  return _REAL(getpid);
}

uptr internal_getppid() {
  DEFINE__REAL(int, getppid);
  return _REAL(getppid);
}

int internal_dlinfo(void* handle, int request, void* p) {
  DEFINE__REAL(int, dlinfo, void* a, int b, void* c);
  return _REAL(dlinfo, handle, request, p);
}

uptr internal_getdents(fd_t fd, void* dirp, unsigned int count) {
  DEFINE__REAL(int, __getdents30, int a, void* b, size_t c);
  return _REAL(__getdents30, fd, dirp, count);
}

uptr internal_lseek(fd_t fd, OFF_T offset, int whence) {
  DEFINE__REAL(uptr, lseek, int, sptr, int);
  return _REAL(lseek, fd, offset, whence);
}

uptr internal_prctl(int option, uptr arg2, uptr arg3, uptr arg4, uptr arg5) {
  Printf("internal_prctl not implemented for NetBSD");
  Die();
  return 0;
}

uptr internal_sigaltstack(const void* ss, void* oss) {
  DEFINE__REAL(int, __sigaltstack14, const void* a, void* b);
  return _REAL(__sigaltstack14, ss, oss);
}

struct shadows_t {
  uptr beg;
  uptr end;
  bool madvise_shadow;
};
struct gaps_t {
  uptr addr;
  uptr size;
  uptr zero_base_shadow_start;
  uptr zero_base_max_shadow_start;
};
struct trampoline_t {
  uptr addr;
  uptr granularity;
};
template <typename T, size_t PageSize>
struct colony {
  struct chunk_header {
    colony* next;
    size_t used_count;
  };
  static constexpr size_t chunk_capacity =
      (PageSize - sizeof(chunk_header)) / sizeof(T);
  chunk_header header;
  T chunk[chunk_capacity];
  void push_back(const T& val) {
    if (header.used_count < chunk_capacity) {
      chunk[header.used_count++] = val;
      return;
    }
    if (!header.next) {
      uptr next = internal_mmap(0, PageSize, PROT_READ | PROT_WRITE,
                                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (next == (uptr)-1)
        Die();
      colony* old = nullptr;
      if (!__atomic_compare_exchange(&header.next, &old, (colony**)&next, false,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        internal_munmap((void*)next, PageSize);
      }
    }
    header.next->push_back(val);
  }
  struct const_iterator {
    const colony* container;
    size_t target;
    const T& operator*() const noexcept { return container->chunk[target]; }
    bool operator==(const const_iterator& rhs) const noexcept {
      return container == rhs.container && target == rhs.target;
    }
    bool operator!=(const const_iterator& rhs) const noexcept {
      return !(*this == rhs);
    }
    const_iterator& operator++() noexcept {
      if (!container)
        return *this;
      if (++target == container->header.used_count) {
        container = container->header.next;
        target = 0;
      }
      return *this;
    }
    const_iterator operator++(int) noexcept {
      auto tmp(*this);
      ++*this;
      return tmp;
    }
  };
  const_iterator begin() const noexcept { return {this, 0}; }
  const_iterator end() const noexcept { return {nullptr, 0}; }
};
static colony<shadows_t, 4096> shadows_list;
static colony<gaps_t, 4096> gaps_list;
static colony<trampoline_t, 4096> trampoline_list;

void RegisterTrampolineBufferForFork(uptr addr, uptr granularity) {
  trampoline_list.push_back({addr, granularity});
}

static bool ReserveShadowMemoryRangeImpl(uptr beg, uptr end,
                                         bool madvise_shadow) {
  CHECK_EQ((beg % GetMmapGranularity()), 0);
  CHECK_EQ(((end + 1) % GetMmapGranularity()), 0);
  uptr size = end - beg + 1;
  if (VirtualAlloc((LPVOID)beg, size, MEM_RESERVE, PAGE_READWRITE) == nullptr &&
      VirtualAlloc((LPVOID)beg, size, MEM_COMMIT, PAGE_READWRITE) == nullptr)
    return false;
  if (madvise_shadow && common_flags()->use_madv_dontdump)
    DontDumpShadowMemory(beg, size);
  return true;
}
static bool ProtectGapImpl(uptr addr, uptr size, uptr zero_base_shadow_start,
                           uptr zero_base_max_shadow_start) {
  if (!size)
    return true;
  void* res = VirtualAlloc((LPVOID)addr, size, MEM_RESERVE, PAGE_NOACCESS);

  if (addr == (uptr)res)
    return true;
  // A few pages at the start of the address space can not be protected.
  // But we really want to protect as much as possible, to prevent this memory
  // being returned as a result of a non-FIXED mmap().
  if (addr == zero_base_shadow_start) {
    uptr step = GetMmapGranularity();
    while (size > step && addr < zero_base_max_shadow_start) {
      addr += step;
      size -= step;
      void* res = VirtualAlloc((LPVOID)addr, size, MEM_RESERVE, PAGE_NOACCESS);
      if (addr == (uptr)res)
        return true;
    }
  }
  return false;
}
int internal_fork() {
  // DEFINE__REAL(int, fork);
  // int ret = _REAL(fork);
  return ::fork();
}

int internal_sysctlbyname(const char* sname, void* oldp, uptr* oldlenp,
                          const void* newp, uptr newlen) {
  DEFINE__REAL(int, sysctlbyname, const char* a, void* b, size_t* c,
               const void* d, size_t e);
  return _REAL(sysctlbyname, sname, oldp, (size_t*)oldlenp, newp,
               (size_t)newlen);
}

uptr internal_sigprocmask(int how, __sanitizer_sigset_t* set,
                          __sanitizer_sigset_t* oldset) {
  DEFINE__REAL(int, sigprocmask, int, const void*, void*);
  return _REAL(sigprocmask, how, set, oldset);
}

void internal_sigfillset(__sanitizer_sigset_t* set) {
  DEFINE__REAL(int, sigfillset, const void* a);
  (void)_REAL(sigfillset, set);
}

void internal_sigemptyset(__sanitizer_sigset_t* set) {
  DEFINE__REAL(int, sigemptyset, const void* a);
  (void)_REAL(sigemptyset, set);
}

void internal_sigdelset(__sanitizer_sigset_t* set, int signo) {
  DEFINE__REAL(int, sigdelset, const void* a, int b);
  (void)_REAL(sigdelset, set, signo);
}

uptr internal_clone(int (*fn)(void*), void* child_stack, int flags, void* arg) {
  DEFINE__REAL(int, clone, int (*a)(void* b), void* c, int d, void* e);

  return _REAL(clone, fn, child_stack, flags, arg);
}

uptr GetPageSize() { return GetMmapGranularity(); }

uptr GetMmapGranularity() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwAllocationGranularity;
}

bool FileExists(const char* filename) {
  // if (ShouldMockFailureToOpen(filename))
  //   return false;
  struct stat st;
  if (::stat(filename, &st))
    return false;
  // Sanity check: filename is a regular file.
  return S_ISREG(st.st_mode);
}

bool DirExists(const char* path) {
  struct stat st;
  if (::stat(path, &st))
    return false;
  return S_ISDIR(st.st_mode);
}

const char* GetEnv(const char* name) {
  return ::getenv(name);
#  if 0
  if (::environ != 0) {
    uptr NameLen = internal_strlen(name);
    for (char **Env = ::environ; *Env != 0; Env++) {
      if (internal_strncmp(*Env, name, NameLen) == 0 && (*Env)[NameLen] == '=')
        return (*Env) + NameLen + 1;
    }
  }
  return 0;  // Not found.
#  endif
}

// Reserve memory range [beg, end].
// We need to use inclusive range because end+1 may not be representable.
void ReserveShadowMemoryRange(uptr beg, uptr end, const char* name,
                              bool madvise_shadow) {
  if (!ReserveShadowMemoryRangeImpl(beg, end, madvise_shadow)) {
    uptr size = end - beg + 1;
    Report(
        "ReserveShadowMemoryRange failed while trying to map 0x%zx bytes. "
        "Perhaps you're using ulimit -v or ulimit -d\n",
        size);
    Die();
  }
  shadows_list.push_back({beg, end, madvise_shadow});
  CHECK_EQ((beg % GetMmapGranularity()), 0);
  CHECK_EQ(((end + 1) % GetMmapGranularity()), 0);
}

void ProtectGap(uptr addr, uptr size, uptr zero_base_shadow_start,
                uptr zero_base_max_shadow_start) {
  if (!ProtectGapImpl(addr, size, zero_base_shadow_start,
                      zero_base_max_shadow_start)) {
    Report(
        "ERROR: Failed to protect the shadow gap. "
        "%s cannot proceed correctly. ABORTING.\n",
        SanitizerToolName);
    DumpProcessMap();
    Die();
  }

  gaps_list.push_back(
      {addr, size, zero_base_shadow_start, zero_base_max_shadow_start});
}

bool MemoryRangeIsAvailable(uptr range_start, uptr range_end) {
  MEMORY_BASIC_INFORMATION mbi;
  CHECK(VirtualQuery((void*)range_start, &mbi, sizeof(mbi)));
  return mbi.Protect == PAGE_NOACCESS &&
         (uptr)mbi.BaseAddress + mbi.RegionSize >= range_end;
}

void CopyShadowsAtForkHandler(void* ph) {
  for (const auto& s : shadows_list) {
    if (ReserveShadowMemoryRangeImpl(s.beg, s.end, s.madvise_shadow)) {
      unsigned char* begin = (unsigned char*)s.beg;
      MEMORY_BASIC_INFORMATION info;
      static_assert(sizeof(MEMORY_BASIC_INFORMATION) ==
                    sizeof(MEMORY_BASIC_INFORMATION64));
      while (begin <= (unsigned char*)s.end &&
             VirtualQueryEx(ph, (void*)begin, &info, sizeof(info))) {
        begin = (unsigned char*)info.BaseAddress;
        if (info.State == MEM_COMMIT) {
          VirtualAlloc(begin, info.RegionSize, MEM_COMMIT, PAGE_READWRITE);
          ReadProcessMemory(ph, begin, begin, info.RegionSize, nullptr);
        }
        begin += info.RegionSize;
      }
    }
  }
  for (const auto& g : gaps_list)
    ProtectGapImpl(g.addr, g.size, g.zero_base_shadow_start,
                   g.zero_base_max_shadow_start);
  for (const auto& t : trampoline_list) {
    VirtualAlloc((void*)t.addr, t.granularity, MEM_RESERVE | MEM_COMMIT,
                 PAGE_EXECUTE_READWRITE);
    ReadProcessMemory(ph, (void*)t.addr, (void*)t.addr, t.granularity, nullptr);
  }
}

static HandleSignalMode GetHandleSignalModeImpl(int signum) {
  switch (signum) {
    case SIGABRT:
      return common_flags()->handle_abort;
    case SIGILL:
      return common_flags()->handle_sigill;
    case SIGTRAP:
      return common_flags()->handle_sigtrap;
    case SIGFPE:
      return common_flags()->handle_sigfpe;
    case SIGSEGV:
      return common_flags()->handle_segv;
    case SIGBUS:
      return common_flags()->handle_sigbus;
  }
  return kHandleSignalNo;
}

HandleSignalMode GetHandleSignalMode(int signum) {
  HandleSignalMode result = GetHandleSignalModeImpl(signum);
  if (result == kHandleSignalYes && !common_flags()->allow_user_segv_handler)
    return kHandleSignalExclusive;
  return result;
}


bool internal_spawn(const char* argv[], const char* envp[], pid_t* pid,
                    fd_t fd_stdin, fd_t fd_stdout) {
    stack_t s;
    sigaltstack(nullptr, &s);
    if (false && s.ss_sp && s.ss_flags == SS_ONSTACK) {
  int f0 = fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC);
  int f1 = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC);
  internal_dup2(fd_stdin, STDIN_FILENO);
  internal_dup2(fd_stdout, STDOUT_FILENO);
  int res = spawnve(_P_NOWAIT, argv[0], argv, envp);
  internal_dup2(f0, STDIN_FILENO);
  internal_dup2(f1, STDOUT_FILENO);
  internal_close(f0);
  internal_close(f1);
  if (res == -1)
    return false;
  *pid = res;
  return true;
    } else {

  // NOTE: Caller ensures that fd_stdin and fd_stdout are not 0, 1, or 2, since
  // this can break communication.
  //
  // NOTE: Caller is responsible for closing fd_stdin after the process has
  // died.

  int res;
  auto fd_closer = at_scope_exit([&] {
    // NOTE: We intentionally do not close fd_stdin since this can
    // cause us to receive a fatal SIGPIPE if the process dies.
    internal_close(fd_stdout);
  });

  // File descriptor actions
  posix_spawn_file_actions_t acts;
  res = posix_spawn_file_actions_init(&acts);
  if (res != 0)
    return false;

  auto acts_cleanup = at_scope_exit([&] {
    posix_spawn_file_actions_destroy(&acts);
  });

  res = posix_spawn_file_actions_adddup2(&acts, fd_stdin, STDIN_FILENO) ||
        posix_spawn_file_actions_adddup2(&acts, fd_stdout, STDOUT_FILENO) ||
        posix_spawn_file_actions_addclose(&acts, fd_stdin) ||
        posix_spawn_file_actions_addclose(&acts, fd_stdout);
  if (res != 0)
    return false;

  // Spawn attributes
  posix_spawnattr_t attrs;
  res = posix_spawnattr_init(&attrs);
  if (res != 0)
    return false;

  auto attrs_cleanup  = at_scope_exit([&] {
    posix_spawnattr_destroy(&attrs);
  });

  // posix_spawn
  char **argv_casted = const_cast<char **>(argv);
  char **envp_casted = const_cast<char **>(envp);
  res = posix_spawn(pid, argv[0], &acts, &attrs, argv_casted, envp_casted);
  if (res != 0)
    return false;

  return true;
    }


}

uptr ReadBinaryName(/*out*/ char* buf, uptr buf_len) {
  InternalMmapVector<wchar_t> binname_utf16(kMaxPathLength);
  int binname_utf16_len =
      GetModuleFileNameW(NULL, &binname_utf16[0], kMaxPathLength);
  if (binname_utf16_len == 0) {
    buf[0] = '\0';
    return 0;
  }
  if (cygwin_conv_path(CCP_WIN_W_TO_POSIX | CCP_ABSOLUTE, &binname_utf16[0],
                       buf, buf_len) != 0)
    return 0;
  return internal_strlen(buf);
}

uptr ReadLongProcessName(/*out*/ char *buf, uptr buf_len) {
  char *tmpbuf;
  uptr tmpsize;
  uptr tmplen;
  if (ReadFileToBuffer("/proc/self/cmdline", &tmpbuf, &tmpsize, &tmplen,
                       1024 * 1024)) {
    internal_strncpy(buf, tmpbuf, buf_len);
    UnmapOrDie(tmpbuf, tmpsize);
    return internal_strlen(buf);
  }
  return ReadBinaryName(buf, buf_len);
}

char** GetArgv() {
  return (char **)cygwin_internal(CW_ARGV);
}

char** GetEnviron() {
  return (char **)cygwin_internal(CW_ENVP);
}

}  // namespace __sanitizer

#endif
