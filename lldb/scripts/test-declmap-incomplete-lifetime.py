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

# ClangUserExpression stores a pointer to the plugin-local persistent-state
# subclass. Its public header needs a forward declaration, not a transitive
# include or a full definition; .cpp uses the concrete type for llvm::cast
# and member calls and must include the defining header itself.
persistent_header = (PLUGIN / "ClangPersistentVariables.h").read_text(
    encoding="utf-8"
)
assert "ClangPersistentVariables *m_clang_state;" in header
namespace_index = header.index("namespace lldb_private {")
forward_index = header.index("class ClangPersistentVariables;")
expression_index = header.index("class ClangUserExpression :")
assert namespace_index < forward_index < expression_index
assert '#include "ClangPersistentVariables.h"' not in header
assert '#include "ClangPersistentVariables.h"' in impl
assert "class ClangPersistentVariables" in persistent_header
assert "llvm::cast<ClangPersistentVariables>" in impl

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
# Utility-function expressions have a *second* DeclMap owner.  Its reset
# and implicitly generated destructor must not instantiate default_delete
# from the header that only knows ClangExpressionHelper's forward declaration.
utility_header = (PLUGIN / "ClangUtilityFunction.h").read_text(encoding="utf-8")
utility_impl = (PLUGIN / "ClangUtilityFunction.cpp").read_text(encoding="utf-8")
utility_class = utility_header.split("class ClangUtilityFunctionHelper", 1)[1]
utility_class = utility_class.split("  };", 1)[0]
assert '#include "ClangExpressionDeclMap.h"' not in utility_header
assert "#include <memory>" in utility_header
assert "std::unique_ptr<ClangExpressionDeclMap> m_expr_decl_map_up;" in utility_class
assert "ClangUtilityFunctionHelper();" in utility_class
assert "~ClangUtilityFunctionHelper() override;" in utility_class
assert "void ResetDeclMap();" in utility_class
assert "m_expr_decl_map_up.reset()" not in utility_header
assert '#include "ClangExpressionDeclMap.h"' in utility_impl
utility_owner = "ClangUtilityFunction::ClangUtilityFunctionHelper::"
assert utility_impl.count(utility_owner + "ClangUtilityFunctionHelper()") == 1
assert utility_impl.count(utility_owner + "~ClangUtilityFunctionHelper()") == 1
assert utility_impl.count(utility_owner + "ResetDeclMap()") == 1
assert "m_expr_decl_map_up.reset();" in utility_impl
assert "std::make_unique<ClangExpressionDeclMap>(" in utility_impl

# An adjacent LLDB helper owns ASTStructExtractor through unique_ptr.
# Keep its construction and destruction in the TU that includes the complete
# ASTStructExtractor class; an inline constructor can trigger unwinding cleanup.
caller_header = (PLUGIN / "ClangFunctionCaller.h").read_text(encoding="utf-8")
caller_impl = (PLUGIN / "ClangFunctionCaller.cpp").read_text(encoding="utf-8")
caller_class = caller_header.split("class ClangFunctionCallerHelper", 1)[1]
caller_class = caller_class.split("  };", 1)[0]
assert "#include <memory>" in caller_header
assert "class ASTStructExtractor;" in caller_header
assert "std::unique_ptr<ASTStructExtractor> m_struct_extractor;" in caller_class
assert "ClangFunctionCallerHelper(ClangFunctionCaller &owner);" in caller_class
assert "~ClangFunctionCallerHelper() override;" in caller_class
assert "ClangFunctionCallerHelper(ClangFunctionCaller &owner) :" not in caller_header
assert '#include "ASTStructExtractor.h"' in caller_impl
caller_owner = "ClangFunctionCaller::ClangFunctionCallerHelper::"
assert caller_impl.count(caller_owner + "ClangFunctionCallerHelper(") == 1
assert caller_impl.count(caller_owner + "~ClangFunctionCallerHelper()") == 1
assert "std::make_unique<ASTStructExtractor>(" in caller_impl

print("LLDB Clang expression incomplete-owner lifetimes: verified")
