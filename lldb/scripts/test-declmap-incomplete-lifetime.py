#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Guard LLDB ClangExpressionDeclMap's incomplete-type ownership boundary.

This test is structural: it does not claim to reproduce an SDK error.
A full native C++/LLDB build is still needed to establish ABI correctness.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PLUGIN = ROOT / "lldb/source/Plugins/ExpressionParser/Clang"
header = (PLUGIN / "ClangUserExpression.h").read_text(encoding="utf-8")
impl = (PLUGIN / "ClangUserExpression.cpp").read_text(encoding="utf-8")

assert '#include "ClangExpressionDeclMap.h"' not in header
assert "class ClangExpressionDeclMap;" in header
assert "#include <memory>" in header

begin = header.index("class ClangUserExpressionHelper")
end = header.index("/// Constructor", begin)
helper = header[begin:end]
assert "std::unique_ptr<ClangExpressionDeclMap> m_expr_decl_map_up;" in helper
assert "ClangUserExpressionHelper(Target &target, bool top_level);" in helper
assert "~ClangUserExpressionHelper() override;" in helper
assert "void ResetDeclMap();" in helper
assert "m_expr_decl_map_up.reset()" not in helper
assert "return m_expr_decl_map_up.get();" in helper

assert '#include "ClangExpressionDeclMap.h"' in impl
ctor = ("ClangUserExpression::ClangUserExpressionHelper::"
        "ClangUserExpressionHelper(")
dtor = ("ClangUserExpression::ClangUserExpressionHelper::"
        "~ClangUserExpressionHelper()")
reset = ("void ClangUserExpression::ClangUserExpressionHelper::"
         "ResetDeclMap()")
assert impl.count(ctor) == 1
assert impl.count(dtor) == 1
assert impl.count(reset) == 1
assert "m_expr_decl_map_up.reset();" in impl[impl.index(reset):impl.index(
    "void ClangUserExpression::ClangUserExpressionHelper::ResetDeclMap(",
    impl.index(reset) + len(reset))]

# Ensure that the actual allocation remains in this same translation unit.
assert "std::make_unique<ClangExpressionDeclMap>(" in impl
print("LLDB incomplete ClangExpressionDeclMap ownership: verified")
