set(LLVM_LINK_COMPONENTS
  AllTargetsAsmParsers
  AllTargetsDescs
  AllTargetsInfos
  BinaryFormat
  Core
  Demangle
  Object
  Option
  Support
  Symbolize
  TargetParser
  TextAPI
  )

set(LLVM_TARGET_DEFINITIONS Opts.td)
tablegen(LLVM Opts.inc -gen-opt-parser-defs)
add_public_tablegen_target(NmOptsTableGen)

set_source_files_properties(llvm-nm.cpp PROPERTIES
  COMPILE_DEFINITIONS "llvm_nm_main=llvm_nm_main_impl"
  )

add_llvm_tool(llvm-nm
  llvm-nm.cpp
  llvm-nm-gnu-compat.cpp

  DEPENDS
  NmOptsTableGen
  intrinsics_gen
  GENERATE_DRIVER
  )

setup_host_tool(llvm-nm LLVM_NM llvm_nm_exe llvm_nm_target)

if(LLVM_INSTALL_BINUTILS_SYMLINKS)
  add_llvm_tool_symlink(nm llvm-nm)
endif()
