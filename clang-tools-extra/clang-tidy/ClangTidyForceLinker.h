//===- ClangTidyForceLinker.h - clang-tidy --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANGTIDYFORCELINKER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CLANGTIDYFORCELINKER_H

#include "clang-tidy-config.h"
#include "llvm/Support/Compiler.h"

#include "ClangTidyForceEnabledLinker.h"

namespace clang::tidy {

#if CLANG_TIDY_ENABLE_STATIC_ANALYZER &&                                       \
    !defined(CLANG_TIDY_DISABLE_STATIC_ANALYZER_CHECKS)
// This anchor is used to force the linker to link the MPIModule.
extern volatile int MPIModuleAnchorSource;
static int LLVM_ATTRIBUTE_UNUSED MPIModuleAnchorDestination =
    MPIModuleAnchorSource;
#endif

} // namespace clang::tidy

#endif
