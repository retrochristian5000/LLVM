# Resolve the linker backends that participate in this LLD build.
#
# Keep the default equivalent to the historical build (all backends), while
# allowing focused consumers to avoid configuring and compiling formats they do
# not ship.  MinGW is a thin driver over the COFF backend, so selecting MinGW
# also selects COFF.
set(LLD_ALL_BACKENDS COFF ELF MachO MinGW wasm)

function(lld_resolve_backends requested out_var)
  set(enabled "${requested}")
  if(NOT enabled OR enabled STREQUAL "all")
    set(enabled ${LLD_ALL_BACKENDS})
  elseif("all" IN_LIST enabled)
    message(FATAL_ERROR "LLD backend 'all' cannot be combined with named backends")
  endif()

  foreach(backend IN LISTS enabled)
    if(NOT backend IN_LIST LLD_ALL_BACKENDS)
      message(FATAL_ERROR "Unknown LLD backend '${backend}'")
    endif()
  endforeach()

  if("MinGW" IN_LIST enabled AND NOT "COFF" IN_LIST enabled)
    list(PREPEND enabled COFF)
  endif()

  list(REMOVE_DUPLICATES enabled)
  set(${out_var} "${enabled}" PARENT_SCOPE)
endfunction()

set(LLD_ENABLE_BACKENDS "all" CACHE STRING
    "Semicolon-separated LLD backends to build, or 'all'")
set_property(CACHE LLD_ENABLE_BACKENDS PROPERTY STRINGS
             all COFF ELF MachO MinGW wasm)

lld_resolve_backends("${LLD_ENABLE_BACKENDS}" LLD_ENABLED_BACKENDS)
message(STATUS "LLD enabled backends: ${LLD_ENABLED_BACKENDS}")
