// REQUIRES: lld-available
/// With lld -opt:ref we can ensure discarded[01] and their profc/profd
/// variables are discarded.

// DEFINE: %{xlink} = %if target={{.*windows-(gnu|cygnus)}} %{-Xlink,%} %else %{%}

// RUN: %clang_profgen -fcoverage-mapping -ffunction-sections -fuse-ld=lld -Wl,%{xlink}-debug:symtab,%{xlink}-opt:noref %S/coverage-linkage.cpp -o %t
// RUN: llvm-nm %t | FileCheck %s --check-prefix=NOGC
// RUN: %clang_profgen -fcoverage-mapping -ffunction-sections -fuse-ld=lld -Wl,%{xlink}-debug:symtab,%{xlink}-opt:ref %S/coverage-linkage.cpp -o %t
// RUN: llvm-nm %t | FileCheck %s --check-prefix=GC

// NOGC:   T {{_Z10|\?}}discarded{{.*}}
// GC-NOT: T {{_Z10|\?}}discarded{{.*}}
