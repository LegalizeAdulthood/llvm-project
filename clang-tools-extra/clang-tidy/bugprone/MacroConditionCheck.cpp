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

static StringRef getTokenName(const Token &Tok) {
  return Tok.is(tok::raw_identifier) ? Tok.getRawIdentifier()
                                     : Tok.getIdentifierInfo()->getName();
}

namespace {

enum class MacroState {
  Undefined,
  DefinedEmpty, // Defined with no value
  DefinedValue, // Defined with a value
  TestedDefined,
  TestedValue,
  If,
  IfDef,
  Else,
  ElIf,
  ElIfDef,
  EndIf
};

struct MacroUsage {
  MacroState State;
  SourceRange Range;
};

struct ConditionMacro {
  ConditionMacro(Token Name, MacroState State, SourceRange Range)
      : Name(getTokenName(Name)), States(1, {State, Range}) {}

  std::string Name;
  std::vector<MacroUsage> States;
};

class MacroConditionCallbacks : public PPCallbacks {
public:
  MacroConditionCallbacks(MacroConditionCheck *Check, const SourceManager &SM,
                          const LangOptions &LangOpts)
      : Check(Check), SM(SM), LangOpts(LangOpts) {}

  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void Defined(const Token &MacroNameTok, const MacroDefinition &MD,
               SourceRange Range) override;
  void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD,
                      const MacroDirective *Undef) override;

  void If(SourceLocation Loc, SourceRange ConditionRange,
          ConditionValueKind ConditionValue) override;
  void Ifdef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD) override;
  void Ifndef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD) override;
  void Elif(SourceLocation Loc, SourceRange ConditionRange,
            ConditionValueKind ConditionValue, SourceLocation IfLoc) override;
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
  void pushState(MacroState State, SourceRange Range);
  void pushStateContainingRange(MacroState State, SourceRange Range);
  void addNewReferencedMacro(Token Tok, MacroState State, SourceRange Range);
  void nameReferenced(Token Tok, MacroState State, SourceRange Range);
  void macrosReferencedInCondition(const SourceRange &ConditionRange);
  SmallVector<ConditionMacro> Macros;
  std::vector<MacroUsage> ActiveCondition;
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
  MacroState State =
      DefineOnly ? MacroState::DefinedEmpty : MacroState::DefinedValue;
  ConditionMacro *It = llvm::find_if(
      Macros, [Name = getTokenName(MacroNameTok)](const ConditionMacro &Macro) {
        return Macro.Name == Name;
      });
  if (It == Macros.end())
    addNewReferencedMacro(MacroNameTok, State, Loc);
  else
    It->States.push_back({State, Loc});
}

void MacroConditionCallbacks::Defined(const Token &MacroNameTok,
                                      const MacroDefinition &MD,
                                      SourceRange Range) {
  ConditionMacro *It = llvm::find_if(
      Macros, [Name = getTokenName(MacroNameTok)](const ConditionMacro &Macro) {
        return Macro.Name == Name;
      });
  if (It == Macros.end())
    addNewReferencedMacro(MacroNameTok, MacroState::TestedDefined, Range);
  else
    It->States.push_back({MacroState::TestedDefined, Range});
}

void MacroConditionCallbacks::MacroUndefined(const Token &MacroNameTok,
                                             const MacroDefinition &MD,
                                             const MacroDirective *Undef) {
  ConditionMacro *It = llvm::find_if(
      Macros, [Name = getTokenName(MacroNameTok)](const ConditionMacro &Macro) {
        return Macro.Name == Name;
      });
  if (It == Macros.end())
    addNewReferencedMacro(MacroNameTok, MacroState::TestedDefined,
                          Undef->getLocation());
  else
    It->States.push_back({MacroState::Undefined, Undef->getLocation()});
}

void MacroConditionCallbacks::If(SourceLocation Loc, SourceRange ConditionRange,
                                 ConditionValueKind ConditionValue) {
  pushStateContainingRange(MacroState::If, ConditionRange);
  macrosReferencedInCondition(ConditionRange);
}

void MacroConditionCallbacks::Ifdef(SourceLocation Loc,
                                    const Token &MacroNameTok,
                                    const MacroDefinition &MD) {
  pushState(MacroState::IfDef, Loc);
  nameReferenced(MacroNameTok, MacroState::TestedDefined, Loc);
}

void MacroConditionCallbacks::Ifndef(SourceLocation Loc,
                                     const Token &MacroNameTok,
                                     const MacroDefinition &MD) {
  pushState(MacroState::IfDef, Loc);
  nameReferenced(MacroNameTok, MacroState::TestedDefined, Loc);
}

void MacroConditionCallbacks::Elifdef(SourceLocation Loc,
                                      const Token &MacroNameTok,
                                      const MacroDefinition &MD) {
  pushState(MacroState::ElIfDef, Loc);
  nameReferenced(MacroNameTok, MacroState::TestedDefined, Loc);
}

void MacroConditionCallbacks::Elifdef(SourceLocation Loc,
                                      SourceRange ConditionRange,
                                      SourceLocation IfLoc) {
  pushState(MacroState::ElIfDef, Loc);
  macrosReferencedInCondition(ConditionRange);
}

void MacroConditionCallbacks::Elifndef(SourceLocation Loc,
                                       const Token &MacroNameTok,
                                       const MacroDefinition &MD) {
  pushState(MacroState::ElIfDef, Loc);
  nameReferenced(MacroNameTok, MacroState::TestedDefined, Loc);
}

void MacroConditionCallbacks::Elifndef(SourceLocation Loc,
                                       SourceRange ConditionRange,
                                       SourceLocation IfLoc) {
  pushState(MacroState::ElIfDef, Loc);
  macrosReferencedInCondition(ConditionRange);
}

void MacroConditionCallbacks::Elif(SourceLocation Loc,
                                   SourceRange ConditionRange,
                                   ConditionValueKind ConditionValue,
                                   SourceLocation IfLoc) {
  pushStateContainingRange(MacroState::ElIf, ConditionRange);
  macrosReferencedInCondition(ConditionRange);
}

void MacroConditionCallbacks::Else(SourceLocation Loc,
                                   SourceLocation /*IfLoc*/) {
  pushState(MacroState::Else, Loc);
}

void MacroConditionCallbacks::Endif(SourceLocation Loc,
                                    SourceLocation /*IfLoc*/) {
  // Drop this condition from the active condition.
  auto StartsCond = [](const MacroUsage &Usage) {
    return Usage.State == MacroState::If || Usage.State == MacroState::IfDef;
  };
  auto Pos = std::prev(
      llvm::find_if(llvm::reverse(ActiveCondition), StartsCond).base());
  ActiveCondition.erase(Pos, ActiveCondition.end());

  for (ConditionMacro &Macro : Macros) {
    auto PrevCond = [](const MacroUsage &Usage) {
      return Usage.State == MacroState::If ||
             Usage.State == MacroState::IfDef ||
             Usage.State == MacroState::Else ||
             Usage.State == MacroState::ElIf ||
             Usage.State == MacroState::ElIfDef;
    };
    // Drop any conditional directives that didn't mention this macro.
    //if (!Macro.States.empty() && PrevCond(Macro.States.back())) {
    //  auto Pos =
    //      llvm::find_if_not(llvm::reverse(Macro.States), PrevCond).base();
    //  if (Pos->State == MacroState::If || Pos->State == MacroState::IfDef)
    //    Macro.States.erase(Pos, Macro.States.end());
    //  else
    //    Macro.States.push_back({MacroState::EndIf, Loc});
    //} else
      Macro.States.push_back({MacroState::EndIf, Loc});
  }
}

void MacroConditionCallbacks::EndOfMainFile() {
  for (ConditionMacro &Macro : Macros) {
    bool HasValue = false;
    bool IsDefined = false;
    bool TestedValue = false;
    bool TestedDefined = false;
    bool HasValueTestedDefined = false;
    SourceLocation HasValueTestedDefinedLoc;
    bool IsDefinedTestedValue = false;
    std::vector<bool> TestedValueStack;
    std::vector<bool> TestedDefinedStack;
    for (const MacroUsage &Usage : Macro.States) {
      switch (Usage.State) {
      case MacroState::Undefined:
        IsDefined = false;
        HasValue = false;
        break;

      case MacroState::DefinedEmpty:
        HasValue = false;
        IsDefined = true;
        break;

      case MacroState::DefinedValue:
        HasValue = true;
        IsDefined = true;
        break;

      case MacroState::TestedDefined:
        if (HasValue && !TestedDefined) {
          HasValueTestedDefined = true;
          HasValueTestedDefinedLoc = Usage.Range.getBegin();
        }
        TestedDefined = true;
        break;

      case MacroState::TestedValue:
        if (!IsDefined)
          Check->diag(Usage.Range.getBegin(),
                      "Macro '%0' value was tested without being defined.")
              << Macro.Name;
        TestedValue = true;
        break;

      case MacroState::If:
        TestedValueStack.push_back(TestedValue);
        TestedDefinedStack.push_back(TestedDefined);
        break;

      case MacroState::IfDef:
        TestedValueStack.push_back(TestedValue);
        TestedDefinedStack.push_back(TestedDefined);
        TestedDefined = true;
        break;

      case MacroState::ElIf:
      case MacroState::Else:
        TestedValue = false;
        TestedDefined = false;
        break;

      case MacroState::ElIfDef:
        TestedDefined = true;
        break;

      case MacroState::EndIf:
        TestedValue = TestedValueStack.back();
        TestedValueStack.pop_back();
        TestedDefined = TestedDefinedStack.back();
        TestedDefinedStack.pop_back();
        break;
      }
    }

    if (HasValueTestedDefined)
      Check->diag(HasValueTestedDefinedLoc,
                  "Macro '%0' defined with a value and checked for definition")
          << Macro.Name;
  }
}

void MacroConditionCallbacks::pushState(MacroState State, SourceRange Range) {
  ActiveCondition.push_back({State, Range});
  for (ConditionMacro &Macro : Macros)
    Macro.States.push_back({State, Range});
}

void MacroConditionCallbacks::pushStateContainingRange(MacroState State,
                                                       SourceRange Range) {
  ActiveCondition.push_back({State, Range});
  for (ConditionMacro &Macro : Macros) {
    if (Macro.States.empty())
      Macro.States.push_back({State, Range});
    else {
      auto Pos = llvm::find_if(llvm::reverse(Macro.States),
                               [Range](const MacroUsage &Usage) {
                                 return !Range.fullyContains(Usage.Range);
                               })
                     .base();
      Macro.States.insert(Pos, {State, Range});
    }
  }
}

void MacroConditionCallbacks::addNewReferencedMacro(Token Tok, MacroState State,
                                                    SourceRange Range) {
  Macros.emplace_back(Tok, MacroState::Undefined, Range);
  llvm::copy(ActiveCondition, std::back_inserter(Macros.back().States));
  Macros.back().States.push_back({State, Range});
}

void MacroConditionCallbacks::nameReferenced(Token Tok, MacroState State,
                                             SourceRange Range) {
  StringRef Name = getTokenName(Tok);
  ConditionMacro *It =
      llvm::find_if(Macros, [Name](const ConditionMacro &Macro) {
        return Macro.Name == Name;
      });
  if (It == Macros.end())
    // Tested without being defined.
    addNewReferencedMacro(Tok, State, Range);
  else
    It->States.push_back({State, Range});
}

void MacroConditionCallbacks::macrosReferencedInCondition(
    const SourceRange &ConditionRange) {
  CharSourceRange CharRange = Lexer::makeFileCharRange(
      CharSourceRange::getTokenRange(ConditionRange), SM, LangOpts);
  std::string Text = Lexer::getSourceText(CharRange, SM, LangOpts).str();
  Lexer Lex(CharRange.getBegin(), LangOpts, Text.data(), Text.data(),
            Text.data() + Text.size());
  Token Tok;
  bool InsideDefined = false;
  bool AtEnd;
  do {
    AtEnd = Lex.LexFromRawLexer(Tok);
    if (!Tok.is(tok::raw_identifier))
      continue;

    if (Tok.getRawIdentifier() == "defined") {
      InsideDefined = true;
      continue;
    }

    // Defined is handled through a callback.
    if (!InsideDefined)
      nameReferenced(Tok, MacroState::TestedValue, ConditionRange);
  } while (!AtEnd);
}

} // namespace

void MacroConditionCheck::registerPPCallbacks(const SourceManager &SM,
                                              Preprocessor *PP,
                                              Preprocessor *ModuleExpanderPP) {
  PP->addPPCallbacks(
      std::make_unique<MacroConditionCallbacks>(this, SM, getLangOpts()));
}
} // namespace bugprone
} // namespace tidy
} // namespace clang
