set(LLVM_LINK_COMPONENTS
  Support
  TargetParser
  )

set(lld_driver_libraries lldCommon)
set(lld_driver_definitions)
set(lld_default_symlinks)

if("COFF" IN_LIST LLD_ENABLED_BACKENDS)
  list(APPEND lld_driver_libraries lldCOFF)
  list(APPEND lld_driver_definitions LLD_ENABLE_COFF_DRIVER)
  list(APPEND lld_default_symlinks lld-link)
endif()
if("ELF" IN_LIST LLD_ENABLED_BACKENDS)
  list(APPEND lld_driver_libraries lldELF)
  list(APPEND lld_driver_definitions LLD_ENABLE_ELF_DRIVER)
  list(APPEND lld_default_symlinks ld.lld)
endif()
if("MachO" IN_LIST LLD_ENABLED_BACKENDS)
  list(APPEND lld_driver_libraries lldMachO)
  list(APPEND lld_driver_definitions LLD_ENABLE_MACHO_DRIVER)
  list(APPEND lld_default_symlinks ld64.lld)
endif()
if("MinGW" IN_LIST LLD_ENABLED_BACKENDS)
  list(APPEND lld_driver_libraries lldMinGW)
  list(APPEND lld_driver_definitions LLD_ENABLE_MINGW_DRIVER)
  if(LLD_DEFAULT_LD_LLD_IS_MINGW AND NOT "ld.lld" IN_LIST lld_default_symlinks)
    list(APPEND lld_default_symlinks ld.lld)
  endif()
endif()
if("wasm" IN_LIST LLD_ENABLED_BACKENDS)
  list(APPEND lld_driver_libraries lldWasm)
  list(APPEND lld_driver_definitions LLD_ENABLE_WASM_DRIVER)
  list(APPEND lld_default_symlinks wasm-ld)
endif()

set_property(SOURCE lld.cpp APPEND PROPERTY
             COMPILE_DEFINITIONS ${lld_driver_definitions})

add_lld_tool(lld
  lld.cpp

  SUPPORT_PLUGINS
  GENERATE_DRIVER
  )
export_executable_symbols_for_plugins(lld)

function(lld_target_link_libraries target type)
  if (TARGET obj.${target})
    target_link_libraries(obj.${target} ${type} ${ARGN})
  endif()

  get_property(LLVM_DRIVER_TOOLS GLOBAL PROPERTY LLVM_DRIVER_TOOLS)
  if(LLVM_TOOL_LLVM_DRIVER_BUILD AND ${target} IN_LIST LLVM_DRIVER_TOOLS)
    set(target llvm-driver)
  endif()

  target_link_libraries(${target} ${type} ${ARGN})
endfunction()

lld_target_link_libraries(lld
  PRIVATE
  ${lld_driver_libraries}
  )

if(NOT LLD_SYMLINKS_TO_CREATE)
  set(LLD_SYMLINKS_TO_CREATE ${lld_default_symlinks})
endif()

foreach(link ${LLD_SYMLINKS_TO_CREATE})
  add_lld_symlink(${link} lld)
endforeach()

if(LLVM_TOOL_LLVM_DRIVER_BUILD)
  set_property(GLOBAL APPEND PROPERTY LLVM_DRIVER_HIDDEN_TOOL_ALIASES_lld ld)
endif()
