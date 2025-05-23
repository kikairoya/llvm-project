// RUN: %clang_cc1 -fsyntax-only -verify=expected,precxx17 %std_cxx11-14 %s
// RUN: %clang_cc1 -fsyntax-only -verify=expected,cxx17 %std_cxx17- %s

// PR4364
template<class T> struct a { // precxx17-note {{here}}
  T b() {
    return typename T::x();
  }
};
struct B {
  typedef B x;
};
B c() {
  a<B> x;
  return x.b();
}

// Some extra tests for invalid cases
template<class T> struct test2 { T b() { return typename T::a; } }; // expected-error{{expected '(' for function-style cast or type construction}}
template<class T> struct test3 { T b() { return typename a; } }; // expected-error{{expected a qualified name after 'typename'}} cxx17-error{{expected '(' for function-style cast or type construction}}
template<class T> struct test4 { T b() { return typename ::a; } }; // precxx17-error{{refers to non-type member}} expected-error{{expected '(' for function-style cast or type construction}}

// PR12884
namespace PR12884_original {
  template <typename T> struct A {
    struct B {
      template <typename U> struct X {};
      typedef int arg;
    };
    struct C {
      typedef B::X<typename B::arg> x; // precxx17-warning{{missing 'typename' prior to dependent type name 'B::X' is a C++20 extension}}
    };
  };

  template <> struct A<int>::B {
    template <int N> struct X {};
    static const int arg = 0;
  };

  A<int>::C::x a;
}
namespace PR12884_half_fixed {
  template <typename T> struct A {
    struct B {
      template <typename U> struct X {};
      typedef int arg;
    };
    struct C {
      typedef typename B::X<typename B::arg> x; // expected-error {{use 'template'}} expected-error {{refers to non-type}}
    };
  };

  template <> struct A<int>::B {
    template <int N> struct X {};
    static const int arg = 0; // expected-note {{here}}
  };

  A<int>::C::x a; // expected-note {{here}}
}
namespace PR12884_fixed {
  template <typename T> struct A {
    struct B {
      template <typename U> struct X {};
      typedef int arg;
    };
    struct C {
      typedef typename B::template X<B::arg> x;
    };
  };

  template <> struct A<int>::B {
    template <int N> struct X {};
    static const int arg = 0;
  };

  A<int>::C::x a; // ok
}

namespace preserve_keyword {
  template <class T> struct A {
    using type = T;
  };

  template <class T> using B = A<T>::type*; // precxx17-warning {{missing 'typename'}}
  void *t1 = *B<int>(); // expected-error {{lvalue of type 'A<int>::type' (aka 'int')}}

  template <class T> using C = typename A<T>::type*;
  void *t2 = *C<int>(); // expected-error {{lvalue of type 'typename A<int>::type' (aka 'int')}}

  using D = A<int>::type*;
  void *t3 = *D(); // expected-error {{lvalue of type 'A<int>::type' (aka 'int')}}

  using D = typename A<int>::type*;
  void *t4 = *D(); // expected-error {{lvalue of type 'typename A<int>::type' (aka 'int')}}
} // namespace preserve_keyword
