# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-apple-darwin %s -o %t1.o

# RUN: %no-fatal-warnings-lld %t1.o -o /dev/null -single_module 2>&1 \
# RUN:     | FileCheck -check-prefix=WARNING %s
# RUN: not %no-fatal-warnings-lld %t1.o -fatal_warnings -o /dev/null \
# RUN:     -single_module 2>&1 | FileCheck -check-prefix=ERROR %s

# RUN: %no-fatal-warnings-lld %t1.o -w -o /dev/null -single_module 2>&1 \
# RUN:     | count 0
# RUN: not %no-fatal-warnings-lld %t1.o -fatal_warnings -w -o /dev/null \
# RUN:     -single_module 2>&1 \
# RUN:     | FileCheck --check-prefix=ERROR %s

# ERROR: error: Option `-single_module' is deprecated
# WARNING: warning: Option `-single_module' is deprecated

# The option groups must explain consequences and next steps instead of
# asking open-ended questions or promising future support.
# RUN: %no-fatal-warnings-lld %t1.o -o /dev/null -classic_linker 2>&1 \
# RUN:     | FileCheck -check-prefix=UNDOCUMENTED %s
# RUN: %no-fatal-warnings-lld %t1.o -o /dev/null -noprebind 2>&1 \
# RUN:     | FileCheck -check-prefix=OBSOLETE %s
# RUN: %no-fatal-warnings-lld %t1.o -o /dev/null -M 2>&1 \
# RUN:     | FileCheck -check-prefix=IGNORED %s
# RUN: %no-fatal-warnings-lld %t1.o -o /dev/null -no_order_data 2>&1 \
# RUN:     | FileCheck -check-prefix=UNIMPLEMENTED %s
# RUN: not %no-fatal-warnings-lld %t1.o -fatal_warnings -M -o /dev/null \
# RUN:     2>&1 | FileCheck -check-prefix=FATAL-IGNORED %s
#
# UNDOCUMENTED: warning: option '-classic_linker' is undocumented in Apple's ld64 and is not supported by ld64.lld
# OBSOLETE: warning: option '-noprebind' is obsolete in Apple's ld64; remove it or use a supported replacement
# IGNORED: warning: option '-M' is accepted for compatibility but ignored; it has no effect on the linked output
# UNIMPLEMENTED: warning: option '-no_order_data' is not fully implemented by ld64.lld; remove it or use a linker that supports the requested behavior
# FATAL-IGNORED: error: option '-M' is accepted for compatibility but ignored

.globl _main
_main:
