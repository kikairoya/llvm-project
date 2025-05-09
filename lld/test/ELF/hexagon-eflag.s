# REQUIRES: hexagon
# RUN: llvm-mc -filetype=obj -mv62 -triple=hexagon-unknown-elf %s -o %t
# RUN: llvm-mc -filetype=obj -mv60 -triple=hexagon-unknown-elf %S/Inputs/hexagon.s -o %t2
# RUN: ld.lld %t2 %t  -o %t3
# RUN: llvm-readelf -h  %t3 | FileCheck %s
## Verify that the largest arch in the input list is selected.
# CHECK: Flags: 0x62

## Verify the arch version when it cannot be inferred from inputs.
# RUN: llvm-ar rcsD %t4
# RUN: ld.lld -m hexagonelf %t4 -o %t5
# RUN: llvm-readelf -h  %t5 | FileCheck --check-prefix=CHECK-EMPTYARCHIVE %s
# CHECK-EMPTYARCHIVE: Flags: 0x68
