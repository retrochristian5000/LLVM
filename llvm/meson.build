project(
  'llvm-nm-meson',
  'cpp',
  meson_version : '>=0.55.0',
  default_options : ['cpp_std=c++17'],
)

# This is intentionally a narrow bootstrap build.  CMake remains the complete
# LLVM build system; Meson currently rebuilds only llvm-nm against a matching
# LLVM library/tool installation exposed through llvm-config.
if meson.is_cross_build()
  error('The initial llvm-nm Meson lane supports native builds only')
endif

llvm_dep = dependency(
  'llvm',
  method : 'config-tool',
  modules : ['all'],
  static : true,
  version : ['>=24.0.0', '<25.0.0'],
)

llvm_bindir = llvm_dep.get_variable(configtool : 'bindir')
llvm_tblgen = find_program(llvm_bindir / 'llvm-tblgen', required : true)
llvm_source_inc = include_directories('include')

message('llvm-nm Meson bootstrap LLVM: ' + llvm_dep.version())
message('llvm-nm Meson llvm-tblgen: ' + llvm_tblgen.full_path())

subdir('tools/llvm-nm')
