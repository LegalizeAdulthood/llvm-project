//===--- MacroConditionCheck.cpp - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MacroConditionCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include <memory>

#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace bugprone {

namespace {

struct ConditionMacro {
  ConditionMacro(Token Name, bool DefinedOnly)
      : Name(Name), DefinedOnly(DefinedOnly) {}

  Token Name;
  bool DefinedOnly;
};

class MacroConditionCallbacks : public PPCallbacks {
public:
  MacroConditionCallbacks(MacroConditionCheck *Check, const SourceManager &SM)
      : Check(Check), SM(SM) {}

  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void Defined(const Token &MacroNameTok, const MacroDefinition &MD,
               SourceRange Range) override;

  void Ifdef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD) override;
  void Ifndef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD) override;
  void Elifdef(SourceLocation Loc, const Token &MacroNameTok,
               const MacroDefinition &MD) override;
  void Elifndef(SourceLocation Loc, const Token &MacroNameTok,
                const MacroDefinition &MD) override;

private:
  void checkValueMacros(const Token &MacroNameTok, const MacroDefinition &MD,
                        SourceLocation Loc);

  SmallVector<ConditionMacro> Macros;
  MacroConditionCheck *Check;
  const SourceManager &SM;
};

void MacroConditionCallbacks::MacroDefined(const Token &MacroNameTok,
                                           const MacroDirective *MD) {
  if (SM.getFilename(MD->getLocation()).empty())
    return;

  const MacroInfo *Info = MD->getMacroInfo();
  if (Info->isFunctionLike() || Info->isBuiltinMacro())
    return;

  bool DefineOnly = Info->tokens().empty();
  Macros.emplace_back(MacroNameTok, DefineOnly);
}

void MacroConditionCallbacks::Defined(const Token &MacroNameTok,
                                      const MacroDefinition &MD,
                                      SourceRange Range) {
  checkValueMacros(MacroNameTok, MD, Range.getBegin());
}

void MacroConditionCallbacks::Ifdef(SourceLocation Loc,
                                    const Token &MacroNameTok,
                                    const MacroDefinition &MD) {
  checkValueMacros(MacroNameTok, MD, Loc);
}

void MacroConditionCallbacks::Ifndef(SourceLocation Loc,
                                     const Token &MacroNameTok,
                                     const MacroDefinition &MD) {
  checkValueMacros(MacroNameTok, MD, Loc);
}

void MacroConditionCallbacks::Elifdef(SourceLocation Loc,
                                      const Token &MacroNameTok,
                                      const MacroDefinition &MD) {
  checkValueMacros(MacroNameTok, MD, Loc);
}

void MacroConditionCallbacks::Elifndef(SourceLocation Loc,
                                       const Token &MacroNameTok,
                                       const MacroDefinition &MD) {
  checkValueMacros(MacroNameTok, MD, Loc);
}

void MacroConditionCallbacks::checkValueMacros(const Token &MacroNameTok, const MacroDefinition &MD, SourceLocation Loc) {
  StringRef Name = MacroNameTok.getIdentifierInfo()->getName();
  StringRef DefinedMsg =
      "Macro '%0' defined here with a value and checked for definition";
  StringRef CheckedMsg =
      "Macro '%0' defined with a value and checked here for definition";
  for (ConditionMacro &Macro : Macros) {
    if (!Macro.DefinedOnly &&
        Macro.Name.getIdentifierInfo()->getName() == Name) {
      Check->diag(MD.getMacroInfo()->getDefinitionLoc(), DefinedMsg) << Name;
      Check->diag(Loc, CheckedMsg) << Name;
    }
  }
}

} // namespace

void MacroConditionCheck::registerPPCallbacks(const SourceManager &SM,
                                              Preprocessor *PP,
                                              Preprocessor *ModuleExpanderPP) {
  PP->addPPCallbacks(std::make_unique<MacroConditionCallbacks>(this, SM));
}
} // namespace bugprone
} // namespace tidy
} // namespace clang
