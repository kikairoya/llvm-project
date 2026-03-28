// RUN: %clang_cc1 -emit-llvm -triple x86_64-mingw  %s -o - | FileCheck %s
// RUN: %clang_cc1 -emit-llvm -triple x86_64-mingw  %s -o - | FileCheck %s --check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -triple i686-mingw    %s -o - | FileCheck %s
// RUN: %clang_cc1 -emit-llvm -triple i686-mingw    %s -o - | FileCheck %s --check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -triple x86_64-cygwin %s -o - | FileCheck %s
// RUN: %clang_cc1 -emit-llvm -triple x86_64-cygwin %s -o - | FileCheck %s --check-prefix=VAR
// RUN: %clang_cc1 -emit-llvm -triple i686-cygwin   %s -o - | FileCheck %s
// RUN: %clang_cc1 -emit-llvm -triple i686-cygwin   %s -o - | FileCheck %s --check-prefix=VAR

#define CAT1(a,b) a##b
#define CAT(a,b) CAT1(a,b)
#define USE(m) __attribute__((used)) auto CAT(anchor_,__LINE__) = m

struct ImportDefinition;
struct ImportDeclaration;
struct NonImportDeclaration;
struct ExpSpecMethod;
struct ImpInstMethod;
struct ExpSpecMethodNonImportDecl;
struct ExpSpecNested;
struct ImpInstNested;
struct SpecializeWholeClass;

template <typename T>
struct Class {
  Class(const Class &) {}
  Class(Class &&) noexcept {}
  // Clang imports inline member functions if and only if it is instantiated
  // explicitly. Therefore, a check for an entity which is/isn't imported should
  // use an inline/non-inline method respectively.
  static void Memfunc();
  static void Inlined() {}
  static int StaticVar;
  static inline int InlineVar = 0;
  struct Nested {
    static void Memfunc();
    static void Specialized() {}
  };
  static void Specialized() {}
};
template <typename T>
void Class<T>::Memfunc() {}
template <typename T>
void Class<T>::Nested::Memfunc() {}
template <typename T>
int Class<T>::StaticVar = 0;

template <typename T> inline Class<T> *ptrOuter = nullptr;
template <typename T> T &&move(T &&x);
template <typename T> T &&move(T &x);

//
// MSVC or pre-C++11 compatibility usage:
// A dllimport-ed explicit instantiation definition makes all members dllimport-ed.
//
template struct __declspec(dllimport) Class<ImportDefinition>;
USE(*ptrOuter<ImportDefinition>);
USE(move(*ptrOuter<ImportDefinition>));
// CHECK: declare dllimport {{(x86_thiscallcc )?}}void @_ZN5ClassI16ImportDefinitionEC1ERKS1_
// CHECK: declare dllimport {{(x86_thiscallcc )?}}void @_ZN5ClassI16ImportDefinitionEC1EOS1_
USE(&Class<ImportDefinition>::Memfunc);
USE(&Class<ImportDefinition>::Inlined);
USE(&Class<ImportDefinition>::StaticVar);
USE(&Class<ImportDefinition>::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI16ImportDefinitionE7MemfuncEv
// CHECK: declare dllimport void @_ZN5ClassI16ImportDefinitionE7InlinedEv
// VAR: @_ZN5ClassI16ImportDefinitionE9StaticVarE = external dllimport global
// VAR: @_ZN5ClassI16ImportDefinitionE9InlineVarE = external dllimport global

//
// Basic usage:
// A dllimport-ed explicit instantiation declaration makes all members dllimport-ed.
//
extern template struct __declspec(dllimport) Class<ImportDeclaration>;
USE(*ptrOuter<ImportDeclaration>);
USE(move(*ptrOuter<ImportDeclaration>));
// CHECK: declare dllimport {{(x86_thiscallcc )?}}void @_ZN5ClassI17ImportDeclarationEC1ERKS1_
// CHECK: declare dllimport {{(x86_thiscallcc )?}}void @_ZN5ClassI17ImportDeclarationEC1EOS1_
USE(&Class<ImportDeclaration>::Memfunc);
USE(&Class<ImportDeclaration>::Inlined);
USE(&Class<ImportDeclaration>::StaticVar);
USE(&Class<ImportDeclaration>::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI17ImportDeclarationE7MemfuncEv
// CHECK: declare dllimport void @_ZN5ClassI17ImportDeclarationE7InlinedEv
// VAR: @_ZN5ClassI17ImportDeclarationE9StaticVarE = external dllimport global
// VAR: @_ZN5ClassI17ImportDeclarationE9InlineVarE = external dllimport global

// dllimport doesn't affect nested classes.
USE(&Class<ImportDeclaration>::Nested::Memfunc);
// CHECK: define linkonce_odr dso_local void @_ZN5ClassI17ImportDeclarationE6Nested7MemfuncEv

//
// Wrong usage:
// A dllimport-ed definition after non-imported declaration doesn't make the instantiation imported.
//
extern template struct Class<NonImportDeclaration>;
template struct __declspec(dllimport) Class<NonImportDeclaration>;
USE(&Class<NonImportDeclaration>::Memfunc);
USE(&Class<NonImportDeclaration>::Inlined);
USE(&Class<NonImportDeclaration>::StaticVar);
USE(&Class<NonImportDeclaration>::InlineVar);
// CHECK: define weak_odr dso_local void @_ZN5ClassI20NonImportDeclarationE7MemfuncEv
// CHECK: define weak_odr dso_local void @_ZN5ClassI20NonImportDeclarationE7InlinedEv
// VAR: @_ZN5ClassI20NonImportDeclarationE9StaticVarE = weak_odr dso_local global
// VAR: @_ZN5ClassI20NonImportDeclarationE9InlineVarE = weak_odr dso_local global

//
// Import after instantiation:
// An implicitly instantiated member prevents other members to be dllimport-ed.
//
USE(&Class<ImpInstMethod>::Specialized);
// CHECK: declare dso_local void @_ZN5ClassI13ImpInstMethodE11SpecializedEv
extern template struct __declspec(dllimport) Class<ImpInstMethod>;
USE(&Class<ImpInstMethod>::Memfunc);
USE(&Class<ImpInstMethod>::StaticVar);
// CHECK: declare dso_local void @_ZN5ClassI13ImpInstMethodE7MemfuncEv
// VAR: @_ZN5ClassI13ImpInstMethodE9StaticVarE = external global

//
// Import after specialization:
// An explicitly specialized member prevents other members to be dllimport-ed.
//
template <> void Class<ExpSpecMethod>::Specialized() {}
// CHECK: define dso_local void @_ZN5ClassI13ExpSpecMethodE11SpecializedEv
extern template struct __declspec(dllimport) Class<ExpSpecMethod>;
USE(&Class<ExpSpecMethod>::Inlined);
USE(&Class<ExpSpecMethod>::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI13ExpSpecMethodE7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecMethodE9InlineVarE = external dllimport global

//
// Imported specialization:
// An imported specialized member doesn't make subsequent instantiations to be dllimport-ed.
//
template <> __declspec(dllimport) void Class<ExpSpecMethodNonImportDecl>::Specialized();
USE(&Class<ExpSpecMethodNonImportDecl>::Specialized);
// CHECK: declare dllimport void @_ZN5ClassI26ExpSpecMethodNonImportDeclE11SpecializedEv

extern template struct Class<ExpSpecMethodNonImportDecl>;
USE(&Class<ExpSpecMethodNonImportDecl>::Memfunc);
USE(&Class<ExpSpecMethodNonImportDecl>::StaticVar);
// CHECK: declare dso_local void @_ZN5ClassI26ExpSpecMethodNonImportDeclE7MemfuncEv
// VAR: @_ZN5ClassI26ExpSpecMethodNonImportDeclE9StaticVarE = external global

//
// Import after instantiation of nested class:
// An implicit instantation of nested class prevents the instantiation of its enclosing type to be dllimport-ed.
//
USE(sizeof(Class<ImpInstNested>::Nested));

extern template struct __declspec(dllimport) Class<ImpInstNested>;
USE(&Class<ImpInstNested>::Inlined);
USE(&Class<ImpInstNested>::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI13ImpInstNestedE7InlinedEv
// VAR: @_ZN5ClassI13ImpInstNestedE9InlineVarE = external dllimport global

//
// Imported specialization of nested class:
// An explicit specialization of nested class prevents the instantiation of its enclosing type to be dllimport-ed.
//
template <>
struct __declspec(dllimport) Class<ExpSpecNested>::Nested {
  static void Memfunc();
  static void Inlined() {}
  static int StaticVar;
  static inline int InlineVar = 0;
};
extern template struct __declspec(dllimport) Class<ExpSpecNested>;
USE(&Class<ExpSpecNested>::Inlined);
USE(&Class<ExpSpecNested>::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI13ExpSpecNestedE7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecNestedE9InlineVarE = external dllimport global

// A fully-specialized nested class isn't an explicit instantation, so an inline method won't be imported.
USE(&Class<ExpSpecNested>::Nested::Memfunc);
USE(&Class<ExpSpecNested>::Nested::Inlined);
USE(&Class<ExpSpecNested>::Nested::StaticVar);
USE(&Class<ExpSpecNested>::Nested::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI13ExpSpecNestedE6Nested7MemfuncEv
// CHECK: define linkonce_odr dso_local void @_ZN5ClassI13ExpSpecNestedE6Nested7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecNestedE6Nested9StaticVarE = external dllimport global
// VAR: @_ZN5ClassI13ExpSpecNestedE6Nested9InlineVarE = external dllimport global

//
// Imported fully specialization:
// An explicit specialization doesn't make its non-exported nested class to be dllimport-ed, the same as non-template class.
//
template <>
struct __declspec(dllimport) Class<SpecializeWholeClass> {
  static void Memfunc();
  static void Inlined() {}
  static int StaticVar;
  static inline int InlineVar = 0;
  struct Nested {
    static void Memfunc();
  };
  struct __declspec(dllimport) ImportedNested {
    static void Memfunc();
  };
};

// An explicitly specialized class isn't an explicit instantation, so an inline method won't be imported.
USE(&Class<SpecializeWholeClass>::Memfunc);
USE(&Class<SpecializeWholeClass>::Inlined);
USE(&Class<SpecializeWholeClass>::StaticVar);
USE(&Class<SpecializeWholeClass>::InlineVar);
// CHECK: declare dllimport void @_ZN5ClassI20SpecializeWholeClassE7MemfuncEv
// CHECK: define linkonce_odr dso_local void @_ZN5ClassI20SpecializeWholeClassE7InlinedEv
// VAR: @_ZN5ClassI20SpecializeWholeClassE9StaticVarE = external dllimport global
// VAR: @_ZN5ClassI20SpecializeWholeClassE9InlineVarE = external dllimport global

USE(&Class<SpecializeWholeClass>::Nested::Memfunc);
USE(&Class<SpecializeWholeClass>::ImportedNested::Memfunc);
// CHECK: declare dso_local void @_ZN5ClassI20SpecializeWholeClassE6Nested7MemfuncEv
// CHECK: declare dllimport void @_ZN5ClassI20SpecializeWholeClassE14ImportedNested7MemfuncEv
