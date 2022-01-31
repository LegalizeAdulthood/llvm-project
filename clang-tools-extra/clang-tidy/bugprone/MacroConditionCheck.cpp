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
  ConditionMacro(Token Name, const MacroDirective *MD, SourceLocation DefinedLoc, bool DefinedOnly)
      : Name(Name), MD(MD), DefinedLoc(DefinedLoc), DefinedOnly(DefinedOnly) {}

  Token Name;
  const MacroDirective *MD;
  SourceLocation DefinedLoc;
  bool DefinedOnly;
  bool DefinedChecked{false};
  std::vector<SourceLocation> DefinedCheckedLocs;
  bool ValueUsed{false};
  std::vector<SourceLocation> ValueUsedLocs;
};

class MacroConditionCallbacks : public PPCallbacks {
public:
  MacroConditionCallbacks(MacroConditionCheck *Check, const SourceManager &SM, const LangOptions &LangOpts)
      : Check(Check), SM(SM), LangOpts(LangOpts) {}

  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void Defined(const Token &MacroNameTok, const MacroDefinition &MD,
               SourceRange Range) override;

  void If(SourceLocation Loc, SourceRange ConditionRange,
          ConditionValueKind ConditionValue) override;
  void Elif(SourceLocation Loc, SourceRange ConditionRange,
            ConditionValueKind ConditionValue, SourceLocation IfLoc) override;
  void Ifdef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD) override;
  void Ifndef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD) override;
  void Elifdef(SourceLocation Loc, const Token &MacroNameTok,
               const MacroDefinition &MD) override;
  void Elifdef(SourceLocation Loc, SourceRange ConditionRange,
               SourceLocation IfLoc) override;
  void Elifndef(SourceLocation Loc, const Token &MacroNameTok,
                const MacroDefinition &MD) override;
  void Elifndef(SourceLocation Loc, SourceRange ConditionRange,
                SourceLocation IfLoc) override;
  void Else(SourceLocation Loc, SourceLocation IfLoc) override;
  void Endif(SourceLocation Loc, SourceLocation IfLoc) override;
  void EndOfMainFile() override;

private:
  void checkValueMacros(const Token &MacroNameTok, const MacroDefinition &MD,
                        SourceLocation Loc);

  SmallVector<ConditionMacro> Macros;
  MacroConditionCheck *Check;
  const SourceManager &SM;
  const LangOptions &LangOpts;
};

void MacroConditionCallbacks::MacroDefined(const Token &MacroNameTok,
                                           const MacroDirective *MD) {
  SourceLocation Loc = MD->getLocation();
  if (SM.getFilename(Loc).empty())
    return;

  const MacroInfo *Info = MD->getMacroInfo();
  if (Info->isFunctionLike() || Info->isBuiltinMacro())
    return;

  bool DefineOnly = Info->tokens().empty();
  Macros.emplace_back(MacroNameTok, MD, Loc, DefineOnly);
}

void MacroConditionCallbacks::Defined(const Token &MacroNameTok,
                                      const MacroDefinition &MD,
                                      SourceRange Range) {
  auto It = llvm::find_if(
      Macros, [Name = MacroNameTok.getIdentifierInfo()->getName()](
                  const ConditionMacro &Macro) {
        return Macro.Name.getIdentifierInfo()->getName() == Name;
      });
  if (It != Macros.end()) {
    It->DefinedChecked = true;
    It->DefinedCheckedLocs.push_back(Range.getBegin());
  }
}

void MacroConditionCallbacks::If(SourceLocation Loc, SourceRange ConditionRange,
                                 ConditionValueKind ConditionValue) {
  CharSourceRange CharRange = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(ConditionRange), SM, LangOpts);
  std::string Text = Lexer::getSourceText(CharRange, SM, LangOpts).str();
  Lexer Lex(CharRange.getBegin(), LangOpts, Text.data(), Text.data(),
            Text.data() + Text.size());
  Token Tok;
  bool InsideDefined = false;
  while (!Lex.LexFromRawLexer(Tok)) {
    if (Tok.is(tok::raw_identifier)) {
      if (Tok.getRawIdentifier() == "defined")
        InsideDefined = true;
      else if (!InsideDefined) {
        auto It = llvm::find_if(Macros, [Name = Tok.getRawIdentifier()](
                                            const ConditionMacro &Macro) {
          return Macro.Name.getIdentifierInfo()->getName() == Name;
        });
        if (It != Macros.end()) {
          It->ValueUsed = true;
          It->ValueUsedLocs.push_back(Loc);
        }
      } else
        InsideDefined = false;
    }
  }
  if (Tok.is(tok::raw_identifier) && !InsideDefined) {
    auto It = llvm::find_if(
        Macros, [Name = Tok.getRawIdentifier()](const ConditionMacro &Macro) {
          return Macro.Name.getIdentifierInfo()->getName() == Name;
        });
    if (It != Macros.end()) {
      It->ValueUsed = true;
      It->ValueUsedLocs.push_back(Loc);
    }
  }
}

void MacroConditionCallbacks::Ifdef(SourceLocation Loc,
                                    const Token &MacroNameTok,
                                    const MacroDefinition &MD) {
  auto It = llvm::find_if(
      Macros, [Name = MacroNameTok.getIdentifierInfo()->getName()](
                  const ConditionMacro &Macro) {
        return Macro.Name.getIdentifierInfo()->getName() == Name;
      });
  if (It != Macros.end()) {
    It->DefinedChecked = true;
    It->DefinedCheckedLocs.push_back(Loc);
  }
}

void MacroConditionCallbacks::Ifndef(SourceLocation Loc,
                                     const Token &MacroNameTok,
                                     const MacroDefinition &MD) {
  auto It = llvm::find_if(
      Macros, [Name = MacroNameTok.getIdentifierInfo()->getName()](
                  const ConditionMacro &Macro) {
        return Macro.Name.getIdentifierInfo()->getName() == Name;
      });
  if (It != Macros.end()) {
    It->DefinedChecked = true;
    It->DefinedCheckedLocs.push_back(Loc);
  }
}

void MacroConditionCallbacks::Elifdef(SourceLocation Loc,
                                      const Token &MacroNameTok,
                                      const MacroDefinition &MD) {
  auto It = llvm::find_if(
      Macros, [Name = MacroNameTok.getIdentifierInfo()->getName()](
                  const ConditionMacro &Macro) {
        return Macro.Name.getIdentifierInfo()->getName() == Name;
      });
  if (It != Macros.end()) {
    It->DefinedChecked = true;
    It->DefinedCheckedLocs.push_back(Loc);
  }
}

void MacroConditionCallbacks::Elifndef(SourceLocation Loc,
                                       const Token &MacroNameTok,
                                       const MacroDefinition &MD) {
  auto It = llvm::find_if(
      Macros, [Name = MacroNameTok.getIdentifierInfo()->getName()](
                  const ConditionMacro &Macro) {
        return Macro.Name.getIdentifierInfo()->getName() == Name;
      });
  if (It != Macros.end()) {
    It->DefinedChecked = true;
    It->DefinedCheckedLocs.push_back(Loc);
  }
}

void MacroConditionCallbacks::Elif(SourceLocation Loc,
    SourceRange ConditionRange, ConditionValueKind ConditionValue,
    SourceLocation IfLoc) {
  std::cout << "#elif\n  " << ConditionRange.getBegin().printToString(SM) << "\n  " << ConditionRange.getEnd().printToString(SM) << '\n';
}

void MacroConditionCallbacks::Elifdef(SourceLocation Loc,
    SourceRange ConditionRange, SourceLocation IfLoc) {
  std::cout << "#elifdef\n  " << ConditionRange.getBegin().printToString(SM) << "\n  " << ConditionRange.getEnd().printToString(SM) << '\n';
}

void MacroConditionCallbacks::Elifndef(SourceLocation Loc,
    SourceRange ConditionRange, SourceLocation IfLoc) {
  std::cout << "#elifndef\n  " << ConditionRange.getBegin().printToString(SM) << "\n  " << ConditionRange.getEnd().printToString(SM) << '\n';
}

void MacroConditionCallbacks::Else(SourceLocation Loc, SourceLocation IfLoc) {
  std::cout << "#else\n  " << Loc.printToString(SM) << "\n  " << IfLoc.printToString(SM) << '\n';
}

void MacroConditionCallbacks::Endif(SourceLocation Loc, SourceLocation IfLoc) {
  std::cout << "#endif\n  " << Loc.printToString(SM) << "\n  " << IfLoc.printToString(SM) << '\n';
}

void MacroConditionCallbacks::EndOfMainFile() {
  StringRef DefinedMsg =
      "Macro '%0' defined here with a value and checked for definition";
  StringRef CheckedMsg =
      "Macro '%0' defined with a value and checked here for definition";
  for (ConditionMacro &Macro : Macros) {
    if (!Macro.DefinedOnly && Macro.DefinedChecked) {
      StringRef Name = Macro.Name.getIdentifierInfo()->getName();
      Check->diag(Macro.MD->getMacroInfo()->getDefinitionLoc(), DefinedMsg)
          << Name;
      for (SourceLocation Loc : Macro.DefinedCheckedLocs)
        Check->diag(Loc, CheckedMsg) << Name;
    }
  }
}

} // namespace

void MacroConditionCheck::registerPPCallbacks(const SourceManager &SM,
                                              Preprocessor *PP,
                                              Preprocessor *ModuleExpanderPP) {
  PP->addPPCallbacks(std::make_unique<MacroConditionCallbacks>(this, SM, getLangOpts()));
}
} // namespace bugprone
} // namespace tidy
} // namespace clang
