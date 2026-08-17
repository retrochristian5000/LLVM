# Keep GNUInstallDirs' library destination aligned with LLVM's own library
# layout when an LLVM package has already supplied LLVM_LIBDIR_SUFFIX.
# Explicit CMAKE_INSTALL_LIBDIR values remain authoritative.
if(NOT DEFINED CMAKE_INSTALL_LIBDIR AND DEFINED LLVM_LIBDIR_SUFFIX)
  set(CMAKE_INSTALL_LIBDIR "lib${LLVM_LIBDIR_SUFFIX}")
endif()

include(GNUInstallDirs)
