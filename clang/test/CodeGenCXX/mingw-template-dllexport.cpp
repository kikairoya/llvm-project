// RUN: %clang_cc1 -emit-llvm -verify -triple i686-w64-mingw32       -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^@_Z.* = }}'--check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -verify -triple i686-w64-mingw32       -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^(define|declare)}}' --check-prefixes=FUNC,FUNC-O0
// RUN: %clang_cc1 -emit-llvm -verify -triple i686-w64-mingw32   -O1 -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^@_Z.* = }}'--check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -verify -triple i686-w64-mingw32   -O1 -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^(define|declare)}}' --check-prefixes=FUNC
// RUN: %clang_cc1 -emit-llvm -verify -triple x86_64-w64-mingw32     -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^@_Z.* = }}'--check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -verify -triple x86_64-w64-mingw32     -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^(define|declare)}}' --check-prefixes=FUNC,FUNC-O0
// RUN: %clang_cc1 -emit-llvm -verify -triple x86_64-w64-mingw32 -O1 -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^@_Z.* = }}'--check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -verify -triple x86_64-w64-mingw32 -O1 -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^(define|declare)}}' --check-prefixes=FUNC
// RUN: %clang_cc1 -emit-llvm -verify -triple i686-pc-cygwin         -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^@_Z.* = }}'--check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -verify -triple i686-pc-cygwin         -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^(define|declare)}}' --check-prefixes=FUNC,FUNC-O0
// RUN: %clang_cc1 -emit-llvm -verify -triple x86_64-pc-cygwin       -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^@_Z.* = }}'--check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -verify -triple x86_64-pc-cygwin       -disable-lifetime-markers %s -o - | FileCheck %s --implicit-check-not='{{^(define|declare)}}' --check-prefixes=FUNC,FUNC-O0

template <class T>
class c {
  // MinGW-GCC does not apply 'dllexport' to inline member function in dll-exported template but clang does from long ago.
  void f() noexcept {}
  void g() noexcept;
  inline static int u = 0;
  static int v;
};
template <class T> void c<T>::g() noexcept {}
template <class T> int c<T>::v = 0;

// VAR: @_ZN1cIiE1uE ={{.*}} dllexport
// VAR: @_ZN1cIiE1vE ={{.*}} dllexport
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIiEaSERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIiEaSEOS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIiE1fEv
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIiE1gEv
template class __declspec(dllexport) c<int>;

// VAR: @_ZN1cIcE1uE ={{.*}} dllexport
// VAR: @_ZN1cIcE1vE ={{.*}} dllexport
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIcEaSERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIcEaSEOS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIcE1fEv
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1cIcE1gEv
extern template class __declspec(dllexport) c<char>;
template class c<char>;

// VAR-NOT: @_ZN1cIdE1uE ={{.*}} dllexport
// VAR:     @_ZN1cIdE1uE =
// VAR-NOT: @_ZN1cIdE1vE ={{.*}} dllexport
// VAR:     @_ZN1cIdE1vE =
// FUNC-NOT: define{{.*}} dllexport{{.*}} @_ZN1cIdE1fEv
// FUNC:     define{{.*}} dso_local{{.*}} @_ZN1cIdE1fEv
// FUNC-NOT: define{{.*}} dllexport{{.*}} @_ZN1cIdE1gEv
// FUNC:     define{{.*}} dso_local{{.*}} @_ZN1cIdE1gEv
extern template class c<double>;
template class __declspec(dllexport) c<double>; // expected-warning {{'dllexport' attribute ignored on explicit instantiation definition}}


template <class T>
class d {
  d() noexcept {}
  d(const d &) noexcept {}
  d(d &&) noexcept {}
  ~d() noexcept {}
  d &operator =(const d &) noexcept { return *this; }
  d &operator =(d &&) noexcept { return *this; }
};

// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEC2Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEC1Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEC2ERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEC1ERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEC2EOS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEC1EOS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiED2Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiED1Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEaSERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1dIiEaSEOS0_
template class __declspec(dllexport) d<int>;


template <class T>
class e {
  e() noexcept;
  e(const e &) noexcept;
  e(e &&) noexcept;
  ~e() noexcept;
  e &operator =(const e &) noexcept;
  e &operator =(e &&) noexcept;
};
template <class T> e<T>::e() noexcept {}
template <class T> e<T>::e(const e<T> &) noexcept {}
template <class T> e<T>::e(e<T> &&) noexcept {}
template <class T> e<T>::~e() noexcept {}
template <class T> e<T> &e<T>::operator =(const e<T> &) noexcept { return *this; }
template <class T> e<T> &e<T>::operator =(e<T> &&) noexcept { return *this; }

// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEC2Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEC1Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEC2ERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEC1ERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEC2EOS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEC1EOS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiED2Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiED1Ev
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEaSERKS0_
// FUNC: define{{.*}} dllexport{{.*}} @_ZN1eIiEaSEOS0_
template class __declspec(dllexport) e<int>;


template <class T>
struct outer {
  outer() noexcept {}
  outer(const outer &) noexcept {}
  outer(outer &&) noexcept {}
  ~outer() noexcept {}
  outer &operator =(const outer &) noexcept { return *this; }
  outer &operator =(outer &&) noexcept { return *this; }
  void f() noexcept {}
  void g() noexcept;
  __attribute__((__exclude_from_explicit_instantiation__)) void h() noexcept;
  inline static int u = 0;
  static int v;
  struct inner {
    inner() noexcept {}
    inner(const inner &) noexcept {}
    inner(inner &&) noexcept {}
    ~inner() noexcept {}
    inner &operator =(const inner &) noexcept { return *this; }
    inner &operator =(inner &&) noexcept { return *this; }
    void f() noexcept {}
    void g() noexcept;
    __attribute__((__exclude_from_explicit_instantiation__)) void h() noexcept;
    inline static int u = 0;
    static int v;
   };
};

template <class T> void outer<T>::g() noexcept {}
template <class T> void outer<T>::h() noexcept {}
template <class T> void outer<T>::inner::g() noexcept {}
template <class T> void outer<T>::inner::h() noexcept {}
template <class T> int outer<T>::v = 0;
template <class T> int outer<T>::inner::v = 0;

// VAR:     @_ZN5outerIiE1uE ={{.*}} dllexport
// VAR:     @_ZN5outerIiE1vE ={{.*}} dllexport
// VAR-NOT: @_ZN5outerIiE5inner1uE ={{.*}} dllexport
// VAR:     @_ZN5outerIiE5inner1uE =
// VAR-NOT: @_ZN5outerIiE5inner1vE ={{.*}} dllexport
// VAR:     @_ZN5outerIiE5inner1vE =
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEC2Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEC1Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEC2ERKS0_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEC1ERKS0_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEC2EOS0_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEC1EOS0_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiED2Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiED1Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEaSERKS0_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiEaSEOS0_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE1fEv
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE1gEv
// FUNC-NOT: define{{.*}} @_ZN5outerIiE1hEv
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerC2Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerC1Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerC2ERKS1_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerC1ERKS1_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerC2EOS1_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerC1EOS1_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerD2Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5innerD1Ev
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5inneraSERKS1_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5inneraSEOS1_
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5inner1fEv
// FUNC:     define{{.*}} dllexport{{.*}} @_ZN5outerIiE5inner1gEv
// FUNC-NOT: define{{.*}} @_ZN5outerIiE5inner1hEv
template struct __declspec(dllexport) outer<int>;

// VAR:     @_ZN5outerIcE1uE = external{{.*}} dllimport
// VAR:     @_ZN5outerIcE1vE = external{{.*}} dllimport
// VAR-NOT: @_ZN5outerIcE5inner1uE = external{{.*}} dllimport
// VAR:     @_ZN5outerIcE5inner1uE =
// VAR-NOT: @_ZN5outerIcE5inner1vE = external{{.*}} dllimport
// VAR:     @_ZN5outerIcE5inner1vE =
// FUNC: define{{.*}} @_Z6anchorv
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcEC1Ev
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcEC1ERKS0_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcEC1EOS0_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcEaSERKS0_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcEaSEOS0_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5innerC1Ev
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5innerC1ERKS1_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5innerC1EOS1_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5inneraSERKS1_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5inneraSEOS1_
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE1fEv
// FUNC:     declare{{.*}} dllimport{{.*}} @_ZN5outerIcE1gEv
// FUNC-O0:  define{{.*}} dso_local{{.*}} @_ZN5outerIcE1hEv
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5inner1fEv
// FUNC:     declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5inner1gEv
// FUNC-O0:  define{{.*}} dso_local{{.*}} @_ZN5outerIcE5inner1hEv
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcE5innerD1Ev
// FUNC-O0:  declare{{.*}} dllimport{{.*}} @_ZN5outerIcED1Ev
extern template struct __declspec(dllimport) outer<char>;

int anchor() noexcept {
  outer<char> oc1;
  outer<char> oc2(oc1);
  outer<char> oc3(static_cast<outer<char> &&>(oc1));
  oc1 = oc2;
  oc2 = static_cast<outer<char> &&>(oc3);
  outer<char>::inner ic1;
  outer<char>::inner ic2(ic1);
  outer<char>::inner ic3(static_cast<outer<char>::inner &&>(ic1));
  ic1 = ic2;
  ic2 = static_cast<outer<char>::inner &&>(ic3);
  oc1.f();
  oc1.g();
  oc1.h();
  ic1.f();
  ic1.g();
  ic1.h();
  return oc1.u + oc1.v + ic1.u + ic1.v;
}
