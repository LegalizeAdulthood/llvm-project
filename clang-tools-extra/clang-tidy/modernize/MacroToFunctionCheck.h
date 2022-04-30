//===--- MacroToFunctionCheck.h - clang-tidy --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_MACROTOFUNCTIONCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_MACROTOFUNCTIONCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace modernize {

class MacroToFunctionCallbacks;

/// Replaces function-like macros that compute a value from their arguments
/// with a template function with a deduced return type.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/modernize-macro-to-function.html
class MacroToFunctionCheck : public ClangTidyCheck {
public:
  MacroToFunctionCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus14 != 0;
  }
  void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
      Preprocessor *ModuleExpanderPP) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  MacroToFunctionCallbacks *PPCallback{};
};

} // namespace modernize
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_MACROTOFUNCTIONCHECK_H
