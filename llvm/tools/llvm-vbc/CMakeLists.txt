# Experimental classic Visual Basic source-to-LLVM IR frontend.
# No VB runtime linkage or target-specific assumptions.
set(LLVM_LINK_COMPONENTS Support)

add_llvm_tool(llvm-vbc
  llvm-vbc.cpp
  )
