set(CLANG_TOOLS_TEST_DEPS
  clang-tidy
  # clang-tidy tests require it.
  clang-resource-headers
  # Clang-tidy tests need clang for building modules.
  clang
)

# Add lit test dependencies.
set(LLVM_UTILS_DEPS
  FileCheck count not
)
foreach(dep ${LLVM_UTILS_DEPS})
  if(TARGET ${dep})
    list(APPEND CLANG_TOOLS_TEST_DEPS ${dep})
  endif()
endforeach()

add_lit_testsuite(check-clang-tidy "Running the Clang Tidy regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CLANG_TOOLS_TEST_DEPS}
  ARGS ${CLANG_TOOLS_TEST_EXTRA_ARGS}
  )

set_target_properties(check-clang-tidy PROPERTIES FOLDER "Clang extra tools' tests")

add_lit_testsuites(CLANG-EXTRA ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CLANG_TOOLS_TEST_DEPS}
  )
