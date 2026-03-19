// RUN: %clang_cc1 -triple x86_64-win32 -fms-extensions -emit-llvm -o - %s | \
// RUN:     FileCheck %s --check-prefixes=MSC
// RUN: %clang_cc1 -triple x86_64-win32 -fms-extensions -emit-llvm -o - %s | \
// RUN:     FileCheck %s --implicit-check-not=notToBeInstantiated
//
// RUN: %clang_cc1 -triple x86_64-mingw                 -emit-llvm -o - %s | \
// RUN:     FileCheck %s --check-prefixes=GNU
// RUN: %clang_cc1 -triple x86_64-mingw                 -emit-llvm -o - %s | \
// RUN:     FileCheck %s --check-prefixes=NEGATIVE-GNU --implicit-check-not=notToBeInstantiated
//
// RUN: %clang_cc1 -triple x86_64-cygwin                -emit-llvm -o - %s | \
// RUN:     FileCheck %s --check-prefixes=GNU
// RUN: %clang_cc1 -triple x86_64-cygwin                -emit-llvm -o - %s | \
// RUN:     FileCheck %s --check-prefixes=NEGATIVE-GNU --implicit-check-not=notToBeInstantiated

// Because --implicit-check-not doesn't work with -DAG checks, negative checks
// are performed on another independent path.

#define EXCLUDE_ATTR __attribute__((exclude_from_explicit_instantiation))

struct NoAttrTag {};
struct WithImportTag {};
struct ImplicitTag {};

// Test that __declspec(dllimport) doesn't instantiate entities marked with
// the exclude_from_explicit_instantiation attribute.
template <class T>
struct BasicCase {
  void noAttrMethod() {}
  EXCLUDE_ATTR void excludedMethod() {}
  EXCLUDE_ATTR __declspec(dllimport) void excludedImportedMethod() {}
  EXCLUDE_ATTR void notToBeInstantiated() {}
  EXCLUDE_ATTR __declspec(dllimport) void notToBeInstantiated_withImport() {}
  void notToBeInstantiated_noAttr() {}
  struct nestedType {
    void noAttrMethod() {}
    __declspec(dllimport) void importedMethod();
    __declspec(dllimport) void importedInlineMethod() {}
    EXCLUDE_ATTR void excludedMethod() {}
    EXCLUDE_ATTR __declspec(dllimport) void excludedImportedMethod();
    EXCLUDE_ATTR __declspec(dllimport) void excludedImportedInlineMethod() {}
    EXCLUDE_ATTR void notToBeInstantiated() {}
    EXCLUDE_ATTR __declspec(dllimport) void notToBeInstantiated_withImport() {}
    void notToBeInstantiated_noAttr() {}
  };
  struct EXCLUDE_ATTR nestedExcludedType {
    void noAttrMethod() {}
    __declspec(dllimport) void importedMethod();
    __declspec(dllimport) void importedInlineMethod() {}
    __declspec(dllimport) void notToBeInstantiated_withImport() {}
    void notToBeInstantiated_noAttr() {}
  };
};

extern template struct __declspec(dllimport) BasicCase<WithImportTag>;
extern template struct BasicCase<NoAttrTag>;

/// Test that an excluded method isn't imported even if the previous explicit
/// instantiation declaration or the method itself is imported.
/// A never-called method `notToBeInstantiated` makes sure that an excluded
/// method isn't instantiated unexpectedly.
void useBaseCase() {
  BasicCase<WithImportTag>().noAttrMethod();
  // MSC-DAG: declare dllimport void @"?noAttrMethod@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI13WithImportTagE12noAttrMethodEv

  BasicCase<WithImportTag>().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI13WithImportTagE14excludedMethodEv

  BasicCase<WithImportTag>().excludedImportedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedMethod@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI13WithImportTagE22excludedImportedMethodEv

  BasicCase<WithImportTag>::nestedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI13WithImportTagE10nestedType12noAttrMethodEv

  BasicCase<WithImportTag>::nestedType().importedMethod();
  // MSC-DAG: declare dllimport void @"?importedMethod@nestedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI13WithImportTagE10nestedType14importedMethodEv

  BasicCase<WithImportTag>::nestedType().importedInlineMethod();
  // MSC-DAG: declare dllimport void @"?importedInlineMethod@nestedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI13WithImportTagE10nestedType20importedInlineMethodEv

  BasicCase<WithImportTag>::nestedType().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@nestedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI13WithImportTagE10nestedType14excludedMethodEv

  BasicCase<WithImportTag>::nestedType().excludedImportedMethod();
  // MSC-DAG: declare dso_local void @"?excludedImportedMethod@nestedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: declare dso_local void @_ZN9BasicCaseI13WithImportTagE10nestedType22excludedImportedMethodEv

  BasicCase<WithImportTag>::nestedType().excludedImportedInlineMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedInlineMethod@nestedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI13WithImportTagE10nestedType28excludedImportedInlineMethodEv

  BasicCase<WithImportTag>::nestedExcludedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedExcludedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI13WithImportTagE18nestedExcludedType12noAttrMethodEv

  BasicCase<WithImportTag>::nestedExcludedType().importedMethod();
  // MSC-DAG: declare dllimport void @"?importedMethod@nestedExcludedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI13WithImportTagE18nestedExcludedType14importedMethodEv

  BasicCase<WithImportTag>::nestedExcludedType().importedInlineMethod();
  // MSC-DAG: declare dllimport void @"?importedInlineMethod@nestedExcludedType@?$BasicCase@UWithImportTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI13WithImportTagE18nestedExcludedType20importedInlineMethodEv

  BasicCase<NoAttrTag>().noAttrMethod();
  // MSC-DAG: declare dso_local void @"?noAttrMethod@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dso_local void @_ZN9BasicCaseI9NoAttrTagE12noAttrMethodEv

  BasicCase<NoAttrTag>().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI9NoAttrTagE14excludedMethodEv

  BasicCase<NoAttrTag>().excludedImportedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedMethod@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI9NoAttrTagE22excludedImportedMethodEv

  BasicCase<NoAttrTag>::nestedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dso_local void @_ZN9BasicCaseI9NoAttrTagE10nestedType12noAttrMethodEv

  BasicCase<NoAttrTag>::nestedType().importedMethod();
  // MSC-DAG: declare dllimport void @"?importedMethod@nestedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI9NoAttrTagE10nestedType14importedMethodEv

  BasicCase<NoAttrTag>::nestedType().importedInlineMethod();
  // MSC-DAG: declare dllimport void @"?importedInlineMethod@nestedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dso_local void @_ZN9BasicCaseI9NoAttrTagE10nestedType20importedInlineMethodEv

  BasicCase<NoAttrTag>::nestedType().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@nestedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI9NoAttrTagE10nestedType14excludedMethodEv

  BasicCase<NoAttrTag>::nestedType().excludedImportedMethod();
  // MSC-DAG: declare dso_local void @"?excludedImportedMethod@nestedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dso_local void @_ZN9BasicCaseI9NoAttrTagE10nestedType22excludedImportedMethodEv

  BasicCase<NoAttrTag>::nestedType().excludedImportedInlineMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedInlineMethod@nestedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI9NoAttrTagE10nestedType28excludedImportedInlineMethodEv

  BasicCase<NoAttrTag>::nestedExcludedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedExcludedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI9NoAttrTagE18nestedExcludedType12noAttrMethodEv

  BasicCase<NoAttrTag>::nestedExcludedType().importedMethod();
  // MSC-DAG: declare dllimport void @"?importedMethod@nestedExcludedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI9NoAttrTagE18nestedExcludedType14importedMethodEv

  BasicCase<NoAttrTag>::nestedExcludedType().importedInlineMethod();
  // MSC-DAG: declare dllimport void @"?importedInlineMethod@nestedExcludedType@?$BasicCase@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI9NoAttrTagE18nestedExcludedType20importedInlineMethodEv

  BasicCase<ImplicitTag>().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE12noAttrMethodEv

  BasicCase<ImplicitTag>().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE14excludedMethodEv

  BasicCase<ImplicitTag>().excludedImportedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedMethod@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE22excludedImportedMethodEv

  BasicCase<ImplicitTag>::nestedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE10nestedType12noAttrMethodEv

  BasicCase<ImplicitTag>::nestedType().importedMethod();
  // MSC-DAG: declare dllimport void @"?importedMethod@nestedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI11ImplicitTagE10nestedType14importedMethodEv

  BasicCase<ImplicitTag>::nestedType().importedInlineMethod();
  // MSC-DAG: declare dllimport void @"?importedInlineMethod@nestedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE10nestedType20importedInlineMethodEv

  BasicCase<ImplicitTag>::nestedType().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@nestedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE10nestedType14excludedMethodEv

  BasicCase<ImplicitTag>::nestedType().excludedImportedMethod();
  // MSC-DAG: declare dso_local void @"?excludedImportedMethod@nestedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: declare dso_local void @_ZN9BasicCaseI11ImplicitTagE10nestedType22excludedImportedMethodEv

  BasicCase<ImplicitTag>::nestedType().excludedImportedInlineMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedInlineMethod@nestedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE10nestedType28excludedImportedInlineMethodEv

  BasicCase<ImplicitTag>::nestedExcludedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedExcludedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE18nestedExcludedType12noAttrMethodEv

  BasicCase<ImplicitTag>::nestedExcludedType().importedMethod();
  // MSC-DAG: declare dllimport void @"?importedMethod@nestedExcludedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN9BasicCaseI11ImplicitTagE18nestedExcludedType14importedMethodEv

  BasicCase<ImplicitTag>::nestedExcludedType().importedInlineMethod();
  // MSC-DAG: declare dllimport void @"?importedInlineMethod@nestedExcludedType@?$BasicCase@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN9BasicCaseI11ImplicitTagE18nestedExcludedType20importedInlineMethodEv
}

// Test that a class-level dllimport attribute won't affect to excluded methods.
template <class T>
struct __declspec(dllimport) ImportWholeTemplate {
  void noAttrMethod() {}
  EXCLUDE_ATTR void excludedMethod() {}
  EXCLUDE_ATTR void notToBeInstantiated() {}
  void notToBeInstantiated_noAttr() {}
  struct nestedType {
    void noAttrMethod() {}
  };
  struct EXCLUDE_ATTR nestedExcludedType {
    void noAttrMethod() {}
    void notToBeInstantiated_noAttr() {}
  };
};

extern template struct ImportWholeTemplate<NoAttrTag>;

void useImportWholeTemplate() {
  ImportWholeTemplate<NoAttrTag>().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@?$ImportWholeTemplate@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN19ImportWholeTemplateI9NoAttrTagE14excludedMethodEv

  ImportWholeTemplate<NoAttrTag>::nestedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedType@?$ImportWholeTemplate@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: declare dllimport void @_ZN19ImportWholeTemplateI9NoAttrTagE10nestedType12noAttrMethodEv

  ImportWholeTemplate<NoAttrTag>::nestedExcludedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedExcludedType@?$ImportWholeTemplate@UNoAttrTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN19ImportWholeTemplateI9NoAttrTagE18nestedExcludedType12noAttrMethodEv

  ImportWholeTemplate<ImplicitTag>().excludedMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?excludedMethod@?$ImportWholeTemplate@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN19ImportWholeTemplateI11ImplicitTagE14excludedMethodEv

  ImportWholeTemplate<ImplicitTag>::nestedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedType@?$ImportWholeTemplate@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN19ImportWholeTemplateI11ImplicitTagE10nestedType12noAttrMethodEv

  ImportWholeTemplate<ImplicitTag>::nestedExcludedType().noAttrMethod();
  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrMethod@nestedExcludedType@?$ImportWholeTemplate@UImplicitTag@@@@QEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN19ImportWholeTemplateI11ImplicitTagE18nestedExcludedType12noAttrMethodEv
}

template <class T>
struct Polymorphic {
  EXCLUDE_ATTR explicit Polymorphic() {}
  virtual void noAttrVirtualMethod() {}
  EXCLUDE_ATTR virtual void excludedVirtualMethod() {}
  EXCLUDE_ATTR __declspec(dllimport) virtual void excludedImportedVirtualMethod() {}
  EXCLUDE_ATTR void notToBeInstantiated() {}
  EXCLUDE_ATTR __declspec(dllimport) void notToBeInstantiated_withImport() {}
  void notToBeInstantiated_noAttr() {}
};

extern template struct __declspec(dllimport) Polymorphic<WithImportTag>;
extern template struct Polymorphic<NoAttrTag>;

/// For the MSVC ABI:
/// A call to an excluded constructor implicitly instantiates the VTable, which
/// triggers the instantiation of all virtual methods, regardless of the exclude
/// attribute. Therefore, the `MSC-DAG` checks are repeated four times for each
/// specialization: once for the VTable and three times for the all three
/// virtual methods of the class template.
///
/// For the Itanium ABI:
/// An implicit instantiation declaration suppresses emitting the VTable, so
/// virtual member functions won't be instantiated. Therefore, for `WithImportTag`
/// and `NoAttrTag` specializations, that have an explicit instantiation
/// declaration, only one `GNU-DAG` check to verify the VTable and three
/// `NEGATIVE-GNU-NOT` checks to ensure the virtual methods are not emitted
/// anywhere are placed. For the `ImplicitTag` specialization, `GNU-DAG` checks
/// are placed four times, since the virtual methods are implicitly instantiated.
///
void usePolymorphic() {
  new Polymorphic<WithImportTag>();
  // MSC-DAG: @"??_S?$Polymorphic@UWithImportTag@@@@6B@" =
  // GNU-DAG: @_ZTV11PolymorphicI13WithImportTagE = external dllimport

  // MSC-DAG: declare dllimport void @"?noAttrVirtualMethod@?$Polymorphic@UWithImportTag@@@@UEAAXXZ"
  // NEGATIVE-GNU-NOT: @_ZN11PolymorphicI13WithImportTagE19noAttrVirtualMethodEv

  // MSC-DAG: define linkonce_odr dso_local void @"?excludedVirtualMethod@?$Polymorphic@UWithImportTag@@@@UEAAXXZ"
  // NEGATIVE-GNU-NOT: @_ZN11PolymorphicI13WithImportTagE21excludedVirtualMethodEv

  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedVirtualMethod@?$Polymorphic@UWithImportTag@@@@UEAAXXZ"
  // NEGATIVE-GNU-NOT: @_ZN11PolymorphicI13WithImportTagE29excludedImportedVirtualMethodEv

  new Polymorphic<NoAttrTag>();
  // MSC-DAG: @"??_7?$Polymorphic@UNoAttrTag@@@@6B@" =
  // GNU-DAG: @_ZTV11PolymorphicI9NoAttrTagE = external

  // MSC-DAG: declare dso_local void @"?noAttrVirtualMethod@?$Polymorphic@UNoAttrTag@@@@UEAAXXZ"
  // NEGATIVE-GNU-NOT: @_ZN11PolymorphicI9NoAttrTagE19noAttrVirtualMethodEv

  // MSC-DAG: define linkonce_odr dso_local void @"?excludedVirtualMethod@?$Polymorphic@UNoAttrTag@@@@UEAAXXZ"
  // NEGATIVE-GNU-NOT: @_ZN11PolymorphicI9NoAttrTagE21excludedVirtualMethodEv

  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedVirtualMethod@?$Polymorphic@UNoAttrTag@@@@UEAAXXZ"
  // NEGATIVE-GNU-NOT: @_ZN11PolymorphicI9NoAttrTagE29excludedImportedVirtualMethodEv

  new Polymorphic<ImplicitTag>();
  // MSC-DAG: @"??_7?$Polymorphic@UImplicitTag@@@@6B@" =
  // GNU-DAG: @_ZTV11PolymorphicI11ImplicitTagE = linkonce_odr dso_local

  // MSC-DAG: define linkonce_odr dso_local void @"?noAttrVirtualMethod@?$Polymorphic@UImplicitTag@@@@UEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN11PolymorphicI11ImplicitTagE19noAttrVirtualMethodEv

  // MSC-DAG: define linkonce_odr dso_local void @"?excludedVirtualMethod@?$Polymorphic@UImplicitTag@@@@UEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN11PolymorphicI11ImplicitTagE21excludedVirtualMethodEv

  // MSC-DAG: define linkonce_odr dso_local void @"?excludedImportedVirtualMethod@?$Polymorphic@UImplicitTag@@@@UEAAXXZ"
  // GNU-DAG: define linkonce_odr dso_local void @_ZN11PolymorphicI11ImplicitTagE29excludedImportedVirtualMethodEv
}

/// Test that the exclude attibute takes precedence over the DLL attrribute in
/// a non-template context also.
struct NonTemplateClass {
  EXCLUDE_ATTR __declspec(dllimport) void excludedImportedMethod();
};
void NonTemplateClass::excludedImportedMethod() {}

struct __declspec(dllimport) NonTemplateImportedClass {
  EXCLUDE_ATTR void excludedMethod();
};
void NonTemplateImportedClass::excludedMethod() {}

void useNonTemplateClass() {
  NonTemplateClass().excludedImportedMethod();
  // MSC-DAG: define dso_local void @"?excludedImportedMethod@NonTemplateClass@@QEAAXXZ"
  // GNU-DAG: define dso_local void @_ZN16NonTemplateClass22excludedImportedMethodEv

  NonTemplateImportedClass().excludedMethod();
  // MSC-DAG: define dso_local void @"?excludedMethod@NonTemplateImportedClass@@QEAAXXZ"
  // GNU-DAG: define dso_local void @_ZN24NonTemplateImportedClass14excludedMethodEv
}

