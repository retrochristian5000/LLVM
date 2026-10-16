//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Interpreter/Interfaces/ScriptedBreakpointInterface.h"
#include "gtest/gtest.h"

using namespace lldb_private;

namespace {
class TestScriptedBreakpointInterface : public ScriptedBreakpointInterface {
public:
  llvm::Expected<StructuredData::GenericSP>
  CreatePluginObject(const ScriptedMetadata &, lldb::BreakpointSP) override {
    return StructuredData::GenericSP{};
  }

  llvm::SmallVector<AbstractMethodRequirement>
  GetAbstractMethodRequirements() const override {
    return {};
  }
};
} // namespace

TEST(ScriptedBreakpointInterfaceTest, DefaultShortHelpIsAbsent) {
  TestScriptedBreakpointInterface derived;
  ScriptedBreakpointInterface &interface = derived;
  EXPECT_FALSE(interface.GetShortHelp().has_value());
}
