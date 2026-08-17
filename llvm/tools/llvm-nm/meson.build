llvm_nm_generated_inc = include_directories('.')

llvm_nm_opts = custom_target(
  'llvm-nm-opts',
  input : 'Opts.td',
  output : 'Opts.inc',
  command : [
    llvm_tblgen,
    '-I', meson.project_source_root() / 'include',
    '-gen-opt-parser-defs',
    '-o', '@OUTPUT@',
    '@INPUT@',
  ],
)

llvm_nm_driver_config = configuration_data()
llvm_nm_driver_config.set('TOOL_NAME', 'llvm_nm')
llvm_nm_driver_config.set('INITLLVM_ARGS', '')
llvm_nm_driver = configure_file(
  input : '../../cmake/modules/llvm-driver-template.cpp.in',
  output : 'llvm-nm-driver.cpp',
  configuration : llvm_nm_driver_config,
)

# CMake applies this rename only to llvm-nm.cpp so the compatibility wrapper can
# own llvm_nm_main() and delegate to llvm_nm_main_impl().  Keep the same ABI
# boundary instead of applying the define to the whole executable.
llvm_nm_impl = static_library(
  'llvm-nm-impl',
  [
    'llvm-nm.cpp',
    llvm_nm_opts,
  ],
  cpp_args : ['-Dllvm_nm_main=llvm_nm_main_impl'],
  include_directories : [
    llvm_source_inc,
    llvm_nm_generated_inc,
  ],
  dependencies : llvm_dep,
  install : false,
)

llvm_nm = executable(
  'llvm-nm',
  [
    'llvm-nm-gnu-compat.cpp',
    llvm_nm_driver,
  ],
  link_whole : llvm_nm_impl,
  include_directories : [
    llvm_source_inc,
    llvm_nm_generated_inc,
  ],
  dependencies : llvm_dep,
  install : true,
)

# Keep the first Meson slice self-checking without importing LLVM's full lit
# infrastructure.  These exercise both the generated driver and the fork's GNU
# compatibility shim.
test('llvm-nm-version', llvm_nm, args : ['--version'])
test(
  'llvm-nm-gnu-compatible-format',
  llvm_nm,
  args : ['--gnu-compatible', '--format=BSD-long-form', '--help'],
)
