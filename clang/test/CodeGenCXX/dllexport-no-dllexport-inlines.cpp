// RUN: %clang_cc1 %s -fms-extensions -triple x86_64-windows-msvc -emit-llvm \
// RUN:     -disable-llvm-passes -O1 -o - -fno-dllexport-inlines |           \
// RUN:   FileCheck --check-prefix=CHECK --check-prefix=NOEXPORTINLINE %s

// RUN: %clang_cc1 %s -fms-extensions -triple x86_64-windows-msvc -emit-llvm \
// RUN:     -disable-llvm-passes -O1 -o -                        |           \
// RUN:   FileCheck --check-prefix=CHECK --check-prefix=EXPORTINLINE   %s

// MinGW doesn't export inline functions except a member function of a template
// instantiation of a class template which is explicitly instanciated, even
// if it contains a local static variable.

// RUN: %clang_cc1 %s -triple x86_64-windows-gnu -emit-llvm \
// RUN:     -disable-llvm-passes -O1 -o - |                 \
// RUN:   FileCheck --check-prefix=MINGW %s


struct __declspec(dllexport) ExportedClass {

  // NOEXPORTINLINE-DAG: define linkonce_odr dso_local void @"?InclassDefFunc@ExportedClass@@
  // EXPORTINLINE-DAG: define weak_odr dso_local dllexport void @"?InclassDefFunc@ExportedClass@@
  // MINGW-DAG: define linkonce_odr dso_local void @_ZN13ExportedClass14InclassDefFuncEv
  void InclassDefFunc() {}

  // CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InclassDefFuncWithStaticVariable@ExportedClass@@QEAAHXZ"
  // MINGW-DAG: define linkonce_odr dso_local noundef i32 @_ZN13ExportedClass32InclassDefFuncWithStaticVariableEv
  int InclassDefFuncWithStaticVariable() {
    // CHECK-DAG: @"?static_variable@?1??InclassDefFuncWithStaticVariable@ExportedClass@@QEAAHXZ@4HA" = weak_odr dso_local dllexport global i32 0, comdat, align 4
    // MINGW-DAG: @_ZZN13ExportedClass32InclassDefFuncWithStaticVariableEvE15static_variable = linkonce_odr dso_local global i32 0, comdat, align 4
    static int static_variable = 0;
    ++static_variable;
    return static_variable;
  }

  // CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InclassDefFuncWithLambdaStaticVariable@ExportedClass@@QEAAHXZ"
  // MINGW-DAG: define linkonce_odr dso_local noundef i32 @_ZN13ExportedClass38InclassDefFuncWithLambdaStaticVariableEv
  int InclassDefFuncWithLambdaStaticVariable() {
    // CHECK-DAG: @"?static_x@?2???R<lambda_1>@?0??InclassDefFuncWithLambdaStaticVariable@ExportedClass@@QEAAHXZ@QEBA?A?<auto>@@XZ@4HA" = weak_odr dso_local dllexport global i32 0, comdat, align 4
    // MINGW-DAG: @_ZZZN13ExportedClass38InclassDefFuncWithLambdaStaticVariableEvENKUlvE_clEvE8static_x = linkonce_odr dso_local global i32 0, comdat, align 4
    return ([]() { static int static_x; return ++static_x; })();
  }

  // NOEXPORTINLINE-DAG: define linkonce_odr dso_local void @"?InlineOutclassDefFunc@ExportedClass@@QEAAXXZ
  // EXPORTINLINE-DAG: define weak_odr dso_local dllexport void @"?InlineOutclassDefFunc@ExportedClass@@QEAAXXZ
  // MINGW-DAG: define linkonce_odr dso_local void @_ZN13ExportedClass21InlineOutclassDefFuncEv
  inline void InlineOutclassDefFunc();

  // CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InlineOutclassDefFuncWithStaticVariable@ExportedClass@@QEAAHXZ"
  // MINGW-DAG: define linkonce_odr dso_local noundef i32 @_ZN13ExportedClass39InlineOutclassDefFuncWithStaticVariableEv
  inline int InlineOutclassDefFuncWithStaticVariable();

  // NOEXPORTINLINE-DAG: define weak_odr dso_local dllexport void @"?InlineLaterOutclassDefFunc@ExportedClass@@QEAAXXZ
  // EXPORTINLINE-DAG: define weak_odr dso_local dllexport void @"?InlineLaterOutclassDefFunc@ExportedClass@@QEAAXXZ
  // MINGW-DAG: define weak_odr dso_local dllexport void @_ZN13ExportedClass26InlineLaterOutclassDefFuncEv
  void InlineLaterOutclassDefFunc();

  // CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InlineLaterOutclassDefFuncWithStaticVariable@ExportedClass@@QEAAHXZ"
  // MINGW-DAG: define weak_odr dso_local dllexport noundef i32 @_ZN13ExportedClass44InlineLaterOutclassDefFuncWithStaticVariableEv
  int InlineLaterOutclassDefFuncWithStaticVariable();

  // CHECK-DAG: define dso_local dllexport void @"?OutoflineDefFunc@ExportedClass@@QEAAXXZ"
  // MINGW-DAG: define dso_local dllexport void @_ZN13ExportedClass16OutoflineDefFuncEv
  void OutoflineDefFunc();

};

void ExportedClass::OutoflineDefFunc() {}

inline void ExportedClass::InlineOutclassDefFunc() {}

inline int ExportedClass::InlineOutclassDefFuncWithStaticVariable() {
  static int static_variable = 0;
  return ++static_variable;
}

inline void ExportedClass::InlineLaterOutclassDefFunc() {}

inline int ExportedClass::InlineLaterOutclassDefFuncWithStaticVariable() {
  static int static_variable = 0;
  return ++static_variable;
}

void ExportedClassUser() {
  ExportedClass a;
  a.InclassDefFunc();
  a.InlineOutclassDefFunc();
  a.InlineLaterOutclassDefFunc();
#if defined(__MINGW32__)
  a.InclassDefFuncWithStaticVariable();
  a.InclassDefFuncWithLambdaStaticVariable();
  a.InlineOutclassDefFuncWithStaticVariable();
  a.InlineLaterOutclassDefFuncWithStaticVariable();
#endif
}

template<typename T>
struct __declspec(dllexport) TemplateExportedClass {
  void InclassDefFunc() {}

  int InclassDefFuncWithStaticVariable() {
    static int static_x = 0;
    return ++static_x;
  }
};

class A11{};
class B22{};

// CHECK-DAG: define weak_odr dso_local dllexport void @"?InclassDefFunc@?$TemplateExportedClass@VA11@@@@QEAAXXZ"
// MINGW-DAG: define weak_odr dso_local dllexport void @_ZN21TemplateExportedClassI3A11E14InclassDefFuncEv
// CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InclassDefFuncWithStaticVariable@?$TemplateExportedClass@VA11@@@@QEAAHXZ"
// MINGW-DAG: define weak_odr dso_local dllexport noundef i32 @_ZN21TemplateExportedClassI3A11E32InclassDefFuncWithStaticVariableEv
// CHECK-DAG: @"?static_x@?2??InclassDefFuncWithStaticVariable@?$TemplateExportedClass@VA11@@@@QEAAHXZ@4HA" = weak_odr dso_local dllexport global i32 0, comdat, align 4
// MINGW-DAG: @_ZZN21TemplateExportedClassI3A11E32InclassDefFuncWithStaticVariableEvE8static_x = weak_odr dso_local dllexport global i32 0, comdat, align 4
template class TemplateExportedClass<A11>;

// NOEXPORTINLINE-DAG: define linkonce_odr dso_local void @"?InclassDefFunc@?$TemplateExportedClass@VB22@@@@QEAAXXZ"
// EXPORTINLINE-DAG: define weak_odr dso_local dllexport void @"?InclassDefFunc@?$TemplateExportedClass@VB22@@@@QEAAXXZ
// MINGW-DAG: define linkonce_odr dso_local void @_ZN21TemplateExportedClassI3B22E14InclassDefFuncEv
// CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InclassDefFuncWithStaticVariable@?$TemplateExportedClass@VB22@@@@QEAAHXZ"
// MINGW-DAG: define linkonce_odr dso_local noundef i32 @_ZN21TemplateExportedClassI3B22E32InclassDefFuncWithStaticVariableEv
// CHECK-DAG: @"?static_x@?2??InclassDefFuncWithStaticVariable@?$TemplateExportedClass@VB22@@@@QEAAHXZ@4HA" = weak_odr dso_local dllexport global i32 0, comdat, align 4
// MINGW-DAG: @_ZZN21TemplateExportedClassI3B22E32InclassDefFuncWithStaticVariableEvE8static_x = linkonce_odr dso_local global i32 0, comdat, align 4
TemplateExportedClass<B22> b22;

void TemplateExportedClassUser() {
  b22.InclassDefFunc();
  b22.InclassDefFuncWithStaticVariable();
}


template<typename T>
struct TemplateNoAttributeClass {
  void InclassDefFunc() {}
  int InclassDefFuncWithStaticLocal() {
    static int static_x;
    return ++static_x;
  }
};

// CHECK-DAG: define weak_odr dso_local dllexport void @"?InclassDefFunc@?$TemplateNoAttributeClass@VA11@@@@QEAAXXZ"
// MINGW-DAG: define weak_odr dso_local dllexport void @_ZN24TemplateNoAttributeClassI3A11E14InclassDefFuncEv
// CHECK-DAG: define weak_odr dso_local dllexport noundef i32 @"?InclassDefFuncWithStaticLocal@?$TemplateNoAttributeClass@VA11
// MINGW-DAG: define weak_odr dso_local dllexport noundef i32 @_ZN24TemplateNoAttributeClassI3A11E29InclassDefFuncWithStaticLocalEv
// CHECK-DAG: @"?static_x@?2??InclassDefFuncWithStaticLocal@?$TemplateNoAttributeClass@VA11@@@@QEAAHXZ@4HA" = weak_odr dso_local dllexport global i32 0, comdat, align 4
// MINGW-DAG: @_ZZN24TemplateNoAttributeClassI3A11E29InclassDefFuncWithStaticLocalEvE8static_x = weak_odr dso_local dllexport global i32 0, comdat, align 4
template class __declspec(dllexport) TemplateNoAttributeClass<A11>;

// CHECK-DAG: define available_externally dllimport void @"?InclassDefFunc@?$TemplateNoAttributeClass@VB22@@@@QEAAXXZ"
// MINGW-DAG: define available_externally dllimport void @_ZN24TemplateNoAttributeClassI3B22E14InclassDefFuncEv
// CHECK-DAG: define available_externally dllimport noundef i32 @"?InclassDefFuncWithStaticLocal@?$TemplateNoAttributeClass@VB22@@@@QEAAHXZ"
// MINGW-DAG: define available_externally dllimport noundef i32 @_ZN24TemplateNoAttributeClassI3B22E29InclassDefFuncWithStaticLocalEv
// CHECK-DAG: @"?static_x@?2??InclassDefFuncWithStaticLocal@?$TemplateNoAttributeClass@VB22@@@@QEAAHXZ@4HA" = available_externally dllimport global i32 0, align 4
extern template class __declspec(dllimport) TemplateNoAttributeClass<B22>;

void TemplateNoAttributeClassUser() {
  TemplateNoAttributeClass<B22> b22;
  b22.InclassDefFunc();
  b22.InclassDefFuncWithStaticLocal();
}

struct __declspec(dllimport) ImportedClass {
  // NOEXPORTINLINE-DAG: define linkonce_odr dso_local void @"?InClassDefFunc@ImportedClass@@QEAAXXZ"
  // EXPORTINLINE-DAG: define available_externally dllimport void @"?InClassDefFunc@ImportedClass@@QEAAXXZ"
  // MINGW-DAG: define linkonce_odr dso_local void @_ZN13ImportedClass14InClassDefFuncEv
  void InClassDefFunc() {}

  // EXPORTINLINE-DAG: define available_externally dllimport noundef i32 @"?InClassDefFuncWithStaticVariable@ImportedClass@@QEAAHXZ"
  // NOEXPORTINLINE-DAG: define linkonce_odr dso_local noundef i32 @"?InClassDefFuncWithStaticVariable@ImportedClass@@QEAAHXZ"
  // MINGW-DAG: define linkonce_odr dso_local noundef i32 @_ZN13ImportedClass32InClassDefFuncWithStaticVariableEv
  int InClassDefFuncWithStaticVariable() {
    // CHECK-DAG: @"?static_variable@?1??InClassDefFuncWithStaticVariable@ImportedClass@@QEAAHXZ@4HA" = available_externally dllimport global i32 0, align 4
    // MINGW-DAG: @_ZZN13ImportedClass32InClassDefFuncWithStaticVariableEvE15static_variable = linkonce_odr dso_local global i32 0, comdat, align 4
    static int static_variable = 0;
    ++static_variable;
    return static_variable;
  }
};

int InClassDefFuncUser() {
  // This is necessary for declare statement of ImportedClass::InClassDefFunc().
  ImportedClass c;
  c.InClassDefFunc();
  return c.InClassDefFuncWithStaticVariable();
}
