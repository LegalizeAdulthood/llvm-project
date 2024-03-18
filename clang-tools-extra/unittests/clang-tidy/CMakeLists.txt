set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

if(CLANG_BUILT_STANDALONE)
  # LLVMTestingSupport library is needed for clang-tidy tests.
  if (EXISTS ${LLVM_MAIN_SRC_DIR}/lib/Testing/Support
      AND NOT TARGET LLVMTestingSupport)
    add_subdirectory(${LLVM_MAIN_SRC_DIR}/lib/Testing/Support
      lib/Testing/Support)
  endif()
endif()

get_filename_component(CLANG_LINT_SOURCE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../../clang-tidy REALPATH)
include_directories(${CLANG_LINT_SOURCE_DIR})
include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/../../include-cleaner/include")

set(CLANG_TIDY_TESTS_MODULE_LIBS "")
function(clang_tidy_enable_module_test short_name module)
  if("all" STREQUAL CLANG_TIDY_ENABLE_MODULES OR "${short_name}" IN_LIST CLANG_TIDY_ENABLE_MODULES)
    list(APPEND CLANG_TIDY_TESTS_MODULE_LIBS obj.${module}Tests)
    set(CLANG_TIDY_TESTS_MODULE_LIBS "${CLANG_TIDY_TESTS_MODULE_LIBS}" PARENT_SCOPE)
    add_subdirectory(${short_name})
  endif()
endfunction()

function(clang_tidy_add_module_test_lib module)
  set(target obj.${module}Tests)
  add_library(${target} OBJECT ${ARGN})
  target_link_libraries(${target} PRIVATE ${module} llvm_gtest)
  target_include_directories(${target} PUBLIC ..)
  set_target_properties(${target} PROPERTIES FOLDER "Object Libraries")
endfunction()

clang_tidy_enable_module_test(google       clangTidyGoogleModule)
clang_tidy_enable_module_test(misc         clangTidyMiscModule)
clang_tidy_enable_module_test(llvm         clangTidyLLVMModule)
clang_tidy_enable_module_test(modernize    clangTidyModernizeModule)
clang_tidy_enable_module_test(objc         clangTidyObjCModule)
clang_tidy_enable_module_test(readability  clangTidyReadabilityModule)
add_extra_unittest(ClangTidyTests
  AddConstTest.cpp
  ClangTidyDiagnosticConsumerTest.cpp
  ClangTidyOptionsTest.cpp
  DeclRefExprUtilsTest.cpp
  GlobListTest.cpp
  IncludeInserterTest.cpp
  NamespaceAliaserTest.cpp
  OptionsProviderTest.cpp
  OverlappingReplacementsTest.cpp
  TransformerClangTidyCheckTest.cpp
  UsingInserterTest.cpp
  )

clang_target_link_libraries(ClangTidyTests
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangSerialization
  clangTooling
  clangToolingCore
  clangTransformer
  clangIncludeCleaner
  )
target_link_libraries(ClangTidyTests
  PRIVATE
  clangTidy
  clangTidyUtils
  ${CLANG_TIDY_TESTS_MODULE_LIBS}
  LLVMTestingAnnotations
  LLVMTestingSupport
  )
