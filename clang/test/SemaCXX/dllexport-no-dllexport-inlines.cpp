// RUN: %clang_cc1 %s -fms-extensions -triple x86_64-win32 -fno-dllexport-inlines -verify=noexport
// RUN: %clang_cc1 %s -fms-extensions -triple x86_64-win32                        -verify=msvc
// RUN: %clang_cc1 %s -fms-extensions -triple x86_64-mingw                        -verify=noexport
// RUN: %clang_cc1 %s -fms-extensions -triple x86_64-cygwin                       -verify=noexport

// msvc-no-diagnostics

__declspec(dllexport) void LaterInlineExportedFunc();
inline void LaterInlineExportedFunc() {}

__declspec(dllimport) void LaterInlineImportedFunc();
inline void LaterInlineImportedFunc() {} // noexport-warning{{'LaterInlineImportedFunc' redeclared inline; 'dllimport' attribute ignored}}

struct __declspec(dllexport) ExportedClass {
  void InclassDefFunc() {}
  inline void InlineOutclassDefFunc();
  void LaterInlineOutclassDefFunc();
  void OutoflineDefFunc();
};

void ExportedClass::OutoflineDefFunc() {}
inline void ExportedClass::InlineOutclassDefFunc() {}
inline void ExportedClass::LaterInlineOutclassDefFunc() {}

struct __declspec(dllimport) ImportedClass {
  void InclassDefFunc() {}
  inline void InlineOutclassDefFunc();
  void LaterInlineOutclassDefFunc();
  void OutoflineDefFunc();
};

inline void ImportedClass::InlineOutclassDefFunc() {}
inline void ImportedClass::LaterInlineOutclassDefFunc() {}
