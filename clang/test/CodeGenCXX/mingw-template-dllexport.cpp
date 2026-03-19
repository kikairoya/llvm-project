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

struct ExportDefinition;
struct ExportDeclaration;
struct NonExportDeclaration;
struct ExpSpecMethod;
struct ImpInstMethod;
struct ExpSpecMethodNonExportDecl;
struct ExpSpecNested;
struct ImpInstNested;
struct ExpSpecNestedMember;
struct ImpInstNestedMember;
struct SpecializeWholeClass;

template <typename T>
struct Class {
  Class(const Class &) {}
  Class(Class &&) noexcept {}
  // Clang exports inline member functions if and only if it is instantiated
  // explicitly. Therefore, a check for an entity which is/isn't exported should
  // use an inline/non-inline method respectively.
  static void Memfunc();
  static void Inlined() {}
  static int StaticVar;
  static inline int InlineVar = 0;
  struct Nested {
    static void Memfunc();
    static void Inlined() {}
    static int StaticVar;
    static inline int InlineVar = 0;
    struct DeepNested {
      static void Memfunc();
      static void Inlined() {}
      static int StaticVar;
      static inline int InlineVar = 0;
    };
    static void Specialized() {}
  };
  static void Specialized() {}
};
template <typename T>
void Class<T>::Memfunc() {}
template <typename T>
void Class<T>::Nested::Memfunc() {}
template <typename T>
void Class<T>::Nested::DeepNested::Memfunc() {}
template <typename T>
int Class<T>::StaticVar = 0;
template <typename T>
int Class<T>::Nested::StaticVar = 0;
template <typename T>
int Class<T>::Nested::DeepNested::StaticVar = 0;

//
// MSVC compatibility usage:
// A dllexport-ed explicit instantiation definition makes all members dllexport-ed.
//
template struct __declspec(dllexport) Class<ExportDefinition>;
// CHECK: define weak_odr dso_local dllexport {{(x86_thiscallcc )?}}void @_ZN5ClassI16ExportDefinitionEC1ERKS1_
// CHECK: define weak_odr dso_local dllexport {{(x86_thiscallcc )?}}void @_ZN5ClassI16ExportDefinitionEC1EOS1_
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI16ExportDefinitionE7MemfuncEv
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI16ExportDefinitionE7InlinedEv
// VAR: @_ZN5ClassI16ExportDefinitionE9StaticVarE = weak_odr dso_local dllexport global
// VAR: @_ZN5ClassI16ExportDefinitionE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI16ExportDefinitionE6Nested7MemfuncEv
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI16ExportDefinitionE6Nested7InlinedEv
// VAR: @_ZN5ClassI16ExportDefinitionE6Nested9StaticVarE = weak_odr dso_local dllexport global
// VAR: @_ZN5ClassI16ExportDefinitionE6Nested9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI16ExportDefinitionE6Nested10DeepNested7MemfuncEv
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI16ExportDefinitionE6Nested10DeepNested7InlinedEv
// VAR: @_ZN5ClassI16ExportDefinitionE6Nested10DeepNested9StaticVarE = weak_odr dso_local dllexport global
// VAR: @_ZN5ClassI16ExportDefinitionE6Nested10DeepNested9InlineVarE = weak_odr dso_local dllexport global

//
// Basic usage:
// A dllexport-ed explicit instantiation declaration makes all members dllexport-ed.
//
extern template struct __declspec(dllexport) Class<ExportDeclaration>;
template struct Class<ExportDeclaration>;
// CHECK: define weak_odr dso_local dllexport {{(x86_thiscallcc )?}}void @_ZN5ClassI17ExportDeclarationEC1ERKS1_
// CHECK: define weak_odr dso_local dllexport {{(x86_thiscallcc )?}}void @_ZN5ClassI17ExportDeclarationEC1EOS1_
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI17ExportDeclarationE7MemfuncEv
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI17ExportDeclarationE7InlinedEv
// VAR: @_ZN5ClassI17ExportDeclarationE9StaticVarE = weak_odr dso_local dllexport global
// VAR: @_ZN5ClassI17ExportDeclarationE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI17ExportDeclarationE6Nested7MemfuncEv
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI17ExportDeclarationE6Nested7InlinedEv
// VAR: @_ZN5ClassI17ExportDeclarationE6Nested9StaticVarE = weak_odr dso_local dllexport global
// VAR: @_ZN5ClassI17ExportDeclarationE6Nested9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI17ExportDeclarationE6Nested10DeepNested7MemfuncEv
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI17ExportDeclarationE6Nested10DeepNested7InlinedEv
// VAR: @_ZN5ClassI17ExportDeclarationE6Nested10DeepNested9StaticVarE = weak_odr dso_local dllexport global
// VAR: @_ZN5ClassI17ExportDeclarationE6Nested10DeepNested9InlineVarE = weak_odr dso_local dllexport global

//
// Wrong usage:
// A dllexport-ed definition after non-exported declaration doesn't make the instantiation exported.
//
extern template struct Class<NonExportDeclaration>;
template struct __declspec(dllexport) Class<NonExportDeclaration>;
// CHECK: define weak_odr dso_local void @_ZN5ClassI20NonExportDeclarationE7MemfuncEv
// VAR: @_ZN5ClassI20NonExportDeclarationE9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI20NonExportDeclarationE6Nested7MemfuncEv
// VAR: @_ZN5ClassI20NonExportDeclarationE6Nested9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI20NonExportDeclarationE6Nested10DeepNested7MemfuncEv
// VAR: @_ZN5ClassI20NonExportDeclarationE6Nested10DeepNested9StaticVarE = weak_odr dso_local global

//
// Export after instantiation:
// An implicitly instantiated member doesn't prevent other members to be dllexport-ed.
//
USE(&Class<ImpInstMethod>::Specialized);
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ImpInstMethodE11SpecializedEv
extern template struct __declspec(dllexport) Class<ImpInstMethod>;
template struct Class<ImpInstMethod>;
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ImpInstMethodE7InlinedEv
// VAR: @_ZN5ClassI13ImpInstMethodE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ImpInstMethodE6Nested7InlinedEv
// VAR: @_ZN5ClassI13ImpInstMethodE6Nested9StaticVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ImpInstMethodE6Nested10DeepNested7InlinedEv
// VAR: @_ZN5ClassI13ImpInstMethodE6Nested10DeepNested9InlineVarE = weak_odr dso_local dllexport global

//
// Export after specialization:
// An explicitly specialized member doesn't prevent other members to be dllexport-ed.
//
template <> void Class<ExpSpecMethod>::Specialized() {}
// CHECK: define dso_local void @_ZN5ClassI13ExpSpecMethodE11SpecializedEv
extern template struct __declspec(dllexport) Class<ExpSpecMethod>;
template struct Class<ExpSpecMethod>;
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ExpSpecMethodE7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecMethodE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ExpSpecMethodE6Nested7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecMethodE6Nested9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ExpSpecMethodE6Nested10DeepNested7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecMethodE6Nested10DeepNested9InlineVarE = weak_odr dso_local dllexport global

//
// Exported specialization:
// An exported specialized member doesn't make subsequent instantiations to be dllexport-ed.
//
template <> __declspec(dllexport) void Class<ExpSpecMethodNonExportDecl>::Specialized() {}
// CHECK: define dso_local dllexport void @_ZN5ClassI26ExpSpecMethodNonExportDeclE11SpecializedEv

extern template struct Class<ExpSpecMethodNonExportDecl>;
template struct Class<ExpSpecMethodNonExportDecl>;
// CHECK: define weak_odr dso_local void @_ZN5ClassI26ExpSpecMethodNonExportDeclE7MemfuncEv
// VAR: @_ZN5ClassI26ExpSpecMethodNonExportDeclE9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI26ExpSpecMethodNonExportDeclE6Nested7MemfuncEv
// VAR: @_ZN5ClassI26ExpSpecMethodNonExportDeclE6Nested9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI26ExpSpecMethodNonExportDeclE6Nested10DeepNested7MemfuncEv
// VAR: @_ZN5ClassI26ExpSpecMethodNonExportDeclE6Nested10DeepNested9StaticVarE = weak_odr dso_local global

//
// Export after instantiation: an implicitly instantiated member of nested class doesn't prevent other members of its enclosing type to be dllexport-ed.
//
USE(&Class<ImpInstNestedMember>::Nested::Specialized);
// CHECK: define weak_odr dso_local void @_ZN5ClassI19ImpInstNestedMemberE6Nested11SpecializedEv

extern template struct __declspec(dllexport) Class<ImpInstNestedMember>;
template struct Class<ImpInstNestedMember>;
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI19ImpInstNestedMemberE7InlinedEv
// VAR: @_ZN5ClassI19ImpInstNestedMemberE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local void @_ZN5ClassI19ImpInstNestedMemberE6Nested7MemfuncEv
// VAR: @_ZN5ClassI19ImpInstNestedMemberE6Nested9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI19ImpInstNestedMemberE6Nested10DeepNested7MemfuncEv
// VAR: @_ZN5ClassI19ImpInstNestedMemberE6Nested10DeepNested9StaticVarE = weak_odr dso_local global

//
// Export after specialization: a specialized member of nested class doesn't prevent other members to be dllexport-ed.
//
template <> void Class<ExpSpecNestedMember>::Nested::Specialized() {}
// CHECK: define dso_local void @_ZN5ClassI19ExpSpecNestedMemberE6Nested11SpecializedEv

extern template struct __declspec(dllexport) Class<ExpSpecNestedMember>;
template struct Class<ExpSpecNestedMember>;
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI19ExpSpecNestedMemberE7InlinedEv
// VAR: @_ZN5ClassI19ExpSpecNestedMemberE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local void @_ZN5ClassI19ExpSpecNestedMemberE6Nested7MemfuncEv
// VAR: @_ZN5ClassI19ExpSpecNestedMemberE6Nested9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI19ExpSpecNestedMemberE6Nested10DeepNested7MemfuncEv
// VAR: @_ZN5ClassI19ExpSpecNestedMemberE6Nested10DeepNested9StaticVarE = weak_odr dso_local global

//
// Export after instantiation of nested class:
// An implicit instantation of nested class doesn't prevent the instantiation of its enclosing type to be dllexport-ed.
//
USE(sizeof(Class<ImpInstNested>::Nested));

extern template struct __declspec(dllexport) Class<ImpInstNested>;
template struct Class<ImpInstNested>;
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ImpInstNestedE7InlinedEv
// VAR: @_ZN5ClassI13ImpInstNestedE9InlineVarE = weak_odr dso_local dllexport global
// CHECK: define weak_odr dso_local void @_ZN5ClassI13ImpInstNestedE6Nested7MemfuncEv
// VAR: @_ZN5ClassI13ImpInstNestedE6Nested9StaticVarE = weak_odr dso_local global
// CHECK: define weak_odr dso_local void @_ZN5ClassI13ImpInstNestedE6Nested10DeepNested7MemfuncEv
// VAR: @_ZN5ClassI13ImpInstNestedE6Nested10DeepNested9StaticVarE = weak_odr dso_local global

//
// Exported specialization of nested class:
// An explicitly specialized nested class doesn't prevent the instantiation of its enclosing type to be dllexport-ed.
//
template <>
struct __declspec(dllexport) Class<ExpSpecNested>::Nested {
  static void Memfunc();
  static void Inlined() {}
  static int StaticVar;
  static inline int InlineVar = 0;
};
// VAR: @_ZN5ClassI13ExpSpecNestedE6Nested9InlineVarE = weak_odr dso_local dllexport global

void Class<ExpSpecNested>::Nested::Memfunc() {}
int Class<ExpSpecNested>::Nested::StaticVar = 0;
// CHECK: define dso_local dllexport void @_ZN5ClassI13ExpSpecNestedE6Nested7MemfuncEv
// VAR: @_ZN5ClassI13ExpSpecNestedE6Nested9StaticVarE = dso_local dllexport global

extern template struct __declspec(dllexport) Class<ExpSpecNested>;
template struct Class<ExpSpecNested>;
// CHECK: define weak_odr dso_local dllexport void @_ZN5ClassI13ExpSpecNestedE7InlinedEv
// VAR: @_ZN5ClassI13ExpSpecNestedE9InlineVarE = weak_odr dso_local dllexport global

// A fully-specialized nested class isn't an explicit instantiation, so an inline method won't be exported.
USE(&Class<ExpSpecNested>::Nested::Inlined);
// CHECK: define linkonce_odr dso_local void @_ZN5ClassI13ExpSpecNestedE6Nested7InlinedEv

//
// Exported fully specialization:
// An explicit specialization doesn't make its non-exported nested class to be dllexport-ed, the same as non-template class.
//
template <>
struct __declspec(dllexport) Class<SpecializeWholeClass> {
  static void Memfunc();
  static void Inlined() {}
  static int StaticVar;
  static inline int InlineVar = 0;
  struct Nested {
    static void Memfunc();
  };
  struct __declspec(dllexport) ExportedNested {
    static void Memfunc();
  };
};
// VAR: @_ZN5ClassI20SpecializeWholeClassE9InlineVarE = weak_odr dso_local dllexport global

void Class<SpecializeWholeClass>::Memfunc() {}
int Class<SpecializeWholeClass>::StaticVar = 0;
// CHECK: define dso_local dllexport void @_ZN5ClassI20SpecializeWholeClassE7MemfuncEv
// VAR: @_ZN5ClassI20SpecializeWholeClassE9StaticVarE = dso_local dllexport global

// An explicitly specialized class isn't an explicit instantiation, so an inline method won't be exported.
USE(&Class<SpecializeWholeClass>::Inlined);
// CHECK: define linkonce_odr dso_local void @_ZN5ClassI20SpecializeWholeClassE7InlinedEv

void Class<SpecializeWholeClass>::Nested::Memfunc() {}
void Class<SpecializeWholeClass>::ExportedNested::Memfunc() {}
// CHECK: define dso_local void @_ZN5ClassI20SpecializeWholeClassE6Nested7MemfuncEv
// CHECK: define dso_local dllexport void @_ZN5ClassI20SpecializeWholeClassE14ExportedNested7MemfuncEv
