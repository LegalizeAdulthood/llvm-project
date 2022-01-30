set(CLANG_TOOLS_TEST_DEPS
  pp-trace
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

add_lit_testsuite(check-pp-trace "Running the PP Trace regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CLANG_TOOLS_TEST_DEPS}
  ARGS ${CLANG_TOOLS_TEST_EXTRA_ARGS}
  )

set_target_properties(check-pp-trace PROPERTIES FOLDER "Clang extra tools' tests")
