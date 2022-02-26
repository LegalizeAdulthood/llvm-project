//===--- PreferScopedEnumCheck.cpp - clang-tidy ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PreferScopedEnumCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {

void PreferScopedEnumCheck::registerMatchers(MatchFinder *Finder) {
  auto Enumerator =
      enumConstantDecl(hasParent(enumDecl(unless(isScoped())).bind("enum")))
          .bind("enumerator");
  Finder->addMatcher(Enumerator, this);
  Finder->addMatcher(declRefExpr(to(Enumerator)).bind("ref"), this);
}

void PreferScopedEnumCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *Ref = Result.Nodes.getNodeAs<DeclRefExpr>("ref")) {
    const auto *Enumerator =
        Result.Nodes.getNodeAs<EnumConstantDecl>("enumerator");
    const auto *Enum = Result.Nodes.getNodeAs<EnumDecl>("enum");
    diag(Ref->getLocation(), "Reference to enumerator '%0' from enum '%1'")
        << Enumerator->getName() << Enum->getName();
  } else {
    const auto *Enumerator =
        Result.Nodes.getNodeAs<EnumConstantDecl>("enumerator");
    const auto *Enum = Result.Nodes.getNodeAs<EnumDecl>("enum");
    diag(Enumerator->getLocation(),
         "Prefer a scoped enum to the unscoped enum '%0'")
        << Enum->getName();
  }
}

} // namespace modernize
} // namespace tidy
} // namespace clang
