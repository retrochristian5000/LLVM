//===- unittest/Support/ProcessTest.cpp -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Process.h"
#include "llvm/Support/Error.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <optional>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

using namespace llvm;
using namespace sys;

TEST(ProcessTest, GetProcessIdTest) {
  const Process::Pid pid = Process::getProcessId();

#ifdef _WIN32
  EXPECT_EQ((DWORD)pid, ::GetCurrentProcessId());
#else
  EXPECT_EQ(pid, ::getpid());
#endif
}


#if defined(__APPLE__) &&                                                     \
    defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) &&                \
    __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1060
TEST(ProcessTest, PreventCoreFilesPreservesCrashHandlersOnModernDarwin) {
  // Limit and signal changes must not leak into the Google Test runner.
  pid_t Child = fork();
  ASSERT_GE(Child, 0);
  if (Child == 0) {
    if (signal(SIGABRT, SIG_IGN) == SIG_ERR)
      _exit(1);
    Process::PreventCoreFiles();

    struct sigaction Action = {};
    if (sigaction(SIGABRT, nullptr, &Action) != 0 ||
        Action.sa_handler != SIG_IGN)
      _exit(2);

    struct rlimit CoreLimit;
    if (getrlimit(RLIMIT_CORE, &CoreLimit) != 0 ||
        CoreLimit.rlim_cur != 0)
      _exit(3);
    _exit(Process::AreCoreFilesPrevented() ? 0 : 4);
  }

  int Status = 0;
  ASSERT_EQ(waitpid(Child, &Status, 0), Child);
  ASSERT_TRUE(WIFEXITED(Status));
  EXPECT_EQ(0, WEXITSTATUS(Status));
}
#endif

TEST(ProcessTest, GetRandomNumberTest) {
  const unsigned r1 = Process::GetRandomNumber();
  const unsigned r2 = Process::GetRandomNumber();
  // It should be extremely unlikely that both r1 and r2 are 0.
  EXPECT_NE((r1 | r2), 0u);
}

#ifdef _MSC_VER
#define setenv(name, var, ignore) _putenv_s(name, var)
#endif

#if HAVE_SETENV || _MSC_VER
TEST(ProcessTest, Basic) {
  setenv("__LLVM_TEST_ENVIRON_VAR__", "abc", true);
  std::optional<std::string> val(Process::GetEnv("__LLVM_TEST_ENVIRON_VAR__"));
  EXPECT_TRUE(val.has_value());
  EXPECT_STREQ("abc", val->c_str());
}

TEST(ProcessTest, None) {
  std::optional<std::string> val(
      Process::GetEnv("__LLVM_TEST_ENVIRON_NO_SUCH_VAR__"));
  EXPECT_FALSE(val.has_value());
}
#endif

#ifdef _WIN32

TEST(ProcessTest, EmptyVal) {
  SetEnvironmentVariableA("__LLVM_TEST_ENVIRON_VAR__", "");
  std::optional<std::string> val(Process::GetEnv("__LLVM_TEST_ENVIRON_VAR__"));
  EXPECT_TRUE(val.has_value());
  EXPECT_STREQ("", val->c_str());
}

TEST(ProcessTest, Wchar) {
  SetEnvironmentVariableW(L"__LLVM_TEST_ENVIRON_VAR__", L"abcdefghijklmnopqrs");
  std::optional<std::string> val(Process::GetEnv("__LLVM_TEST_ENVIRON_VAR__"));
  EXPECT_TRUE(val.has_value());
  EXPECT_STREQ("abcdefghijklmnopqrs", val->c_str());
}
#endif

class PageSizeTest : public testing::Test {
  Triple Host;

protected:
  PageSizeTest() : Host(Triple::normalize(sys::getProcessTriple())) {}

  bool isSupported() const {
    // For now just on X86-64 and Aarch64. This can be expanded in the future.
    return (Host.getArch() == Triple::x86_64 ||
            Host.getArch() == Triple::aarch64) &&
           Host.getOS() == Triple::Linux;
  }

  bool pageSizeAsExpected(unsigned PageSize) const {
    switch (Host.getArch()) {
    case Triple::x86_64:
      return PageSize == 4096;
    case Triple::aarch64:
      // supported granule sizes are 4k, 16k and 64k
      return PageSize == 4096 || PageSize == 16384 || PageSize == 65536;
    default:
      llvm_unreachable("unexpected arch!");
    }
  }
};

TEST_F(PageSizeTest, PageSize) {
  if (!isSupported())
    GTEST_SKIP();

  llvm::Expected<unsigned> Result = llvm::sys::Process::getPageSize();
  ASSERT_FALSE(!Result);
  ASSERT_TRUE(pageSizeAsExpected(*Result));
}

} // end anonymous namespace
