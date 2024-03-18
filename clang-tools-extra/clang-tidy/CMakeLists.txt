set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/clang-tidy-config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/clang-tidy-config.h)
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_clang_library(clangTidy
  ClangTidy.cpp
  ClangTidyCheck.cpp
  ClangTidyModule.cpp
  ClangTidyDiagnosticConsumer.cpp
  ClangTidyOptions.cpp
  ClangTidyProfiling.cpp
  ExpandModularHeadersPPCallbacks.cpp
  GlobList.cpp
  NoLintDirectiveHandler.cpp

  DEPENDS
  ClangSACheckers
  omp_gen
  ClangDriverOptions
  )

clang_target_link_libraries(clangTidy
  PRIVATE
  clangAST
  clangASTMatchers
  clangAnalysis
  clangBasic
  clangFormat
  clangFrontend
  clangLex
  clangRewrite
  clangSema
  clangSerialization
  clangTooling
  clangToolingCore
  )

if(CLANG_TIDY_ENABLE_STATIC_ANALYZER)
  clang_target_link_libraries(clangTidy
    PRIVATE
    clangStaticAnalyzerCore
    clangStaticAnalyzerFrontend
  )
endif()

set(CLANG_TIDY_ENABLE_MODULES "all" CACHE STRING "List of clang-tidy check modules to enable; 'all' builds all available check modules.")
set(CLANG_TIDY_ALL_MODULES
  clangTidyAndroidModule
  clangTidyAbseilModule
  clangTidyAlteraModule
  clangTidyBoostModule
  clangTidyBugproneModule
  clangTidyCERTModule
  clangTidyConcurrencyModule
  clangTidyCppCoreGuidelinesModule
  clangTidyDarwinModule
  clangTidyFuchsiaModule
  clangTidyGoogleModule
  clangTidyHICPPModule
  clangTidyLinuxKernelModule
  clangTidyLLVMModule
  clangTidyLLVMLibcModule
  clangTidyMiscModule
  clangTidyModernizeModule
  clangTidyObjCModule
  clangTidyOpenMPModule
  clangTidyPerformanceModule
  clangTidyPortabilityModule
  clangTidyReadabilityModule
  clangTidyZirconModule
  )
# build associative map from lower case module short name to actual module name
set(short_module_names "")
foreach(module ${CLANG_TIDY_ALL_MODULES})
  string(REGEX REPLACE "clangTidy(.*)Module" "\\1" module_name ${module})
  string(TOLOWER "${module_name}" short_name)
  set(clangTidy${short_name}Target ${module})
  set(clangTidy${short_name}Module "${module_name}Module")
  list(APPEND short_module_names "${short_name}")
endforeach()
string(TOLOWER "${CLANG_TIDY_ENABLE_MODULES}" CLANG_TIDY_ENABLE_MODULES)
set(module_anchors "")
if("all" IN_LIST CLANG_TIDY_ENABLE_MODULES)
  set(ALL_CLANG_TIDY_CHECKS ${CLANG_TIDY_ALL_MODULES})
  set(module_anchors ${short_module_names})
else()
  foreach(short_name ${CLANG_TIDY_ENABLE_MODULES})
    if(${short_name} IN_LIST short_module_names)
      list(APPEND ALL_CLANG_TIDY_CHECKS ${clangTidy${short_name}Target})
      list(APPEND module_anchors ${short_name})
    endif()
  endforeach()
endif()
if(CLANG_TIDY_ENABLE_STATIC_ANALYZER)
  list(APPEND ALL_CLANG_TIDY_CHECKS clangTidyMPIModule)
endif()
set(clang_tidy_module_anchor_snippet [=[
// This anchor is used to force the linker to link the @module_name@.
extern volatile int @module_name@AnchorSource;
static int LLVM_ATTRIBUTE_UNUSED @module_name@AnchorDestination =
    @module_name@AnchorSource;

]=])
set(module_anchor_header "")
function(add_check_module short_name)
  if("all" IN_LIST CLANG_TIDY_ENABLE_MODULES OR "${short_name}" IN_LIST CLANG_TIDY_ENABLE_MODULES)
    if(NOT "all" STREQUAL CLANG_TIDY_ENABLE_MODULES)
      message(STATUS "${clangTidy${short_name}Target} is enabled")
    endif()
    add_subdirectory(${short_name})
    set(module_name ${clangTidy${short_name}Module})
    string(CONFIGURE "${clang_tidy_module_anchor_snippet}" anchor)
    string(APPEND module_anchor_header "${anchor}")
    set(module_anchor_header "${module_anchor_header}" PARENT_SCOPE)
  else()
    if(NOT "all" STREQUAL CLANG_TIDY_ENABLE_MODULES)
      message(STATUS "${clangTidy${short_name}Target} is NOT enabled")
    endif()
  endif()
endfunction()

# Checks.
# If you add a check, also add it to ClangTidyForceLinker.h in this directory.
add_check_module(android)
add_check_module(abseil)
add_check_module(altera)
add_check_module(boost)
add_check_module(bugprone)
add_check_module(cert)
add_check_module(concurrency)
add_check_module(cppcoreguidelines)
add_check_module(darwin)
add_check_module(fuchsia)
add_check_module(google)
add_check_module(hicpp)
add_check_module(linuxkernel)
add_check_module(llvm)
add_check_module(llvmlibc)
add_check_module(misc)
add_check_module(modernize)
if(CLANG_TIDY_ENABLE_STATIC_ANALYZER)
  add_subdirectory(mpi)
endif()
add_check_module(objc)
add_check_module(openmp)
add_check_module(performance)
add_check_module(portability)
add_check_module(readability)
add_check_module(zircon)

configure_file(ClangTidyForceEnabledLinker.h.in ClangTidyForceEnabledLinker.h @ONLY)

# Other subtargets. These may reference ALL_CLANG_TIDY_CHECKS
# and must be below its definition.
add_subdirectory(plugin)
add_subdirectory(tool)
add_subdirectory(utils)

if (NOT LLVM_INSTALL_TOOLCHAIN_ONLY)
  install(DIRECTORY .
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/clang-tidy"
    COMPONENT clang-tidy-headers
    FILES_MATCHING
    PATTERN "*.h"
    )
  add_custom_target(clang-tidy-headers)
  set_target_properties(clang-tidy-headers PROPERTIES FOLDER "Misc")
  if(NOT LLVM_ENABLE_IDE)
    add_llvm_install_targets(install-clang-tidy-headers
                             DEPENDS clang-tidy-headers
                             COMPONENT clang-tidy-headers)
  endif()
endif()
