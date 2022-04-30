//===--- MacroToFunctionCheck.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MacroToFunctionCheck.h"
#include "FunctionExpressionMatcher.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

#include <memory>
#include <utility>
#include <vector>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace modernize {

class MacroToFunctionCallbacks : public PPCallbacks {
public:
  MacroToFunctionCallbacks(MacroToFunctionCheck *Check,
                           const LangOptions &LangOpts, const SourceManager &SM)
      : Check(Check), LangOpts(LangOpts), SM(SM) {}
  ~MacroToFunctionCallbacks() override = default;

  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD,
                      const MacroDirective *Undef) override;
  void EndOfMainFile() override;

  void invalidateRange(const SourceRange &Range);

private:
  MacroToFunctionCheck *Check;
  const LangOptions &LangOpts;
  const SourceManager &SM;

  struct FunctionMacro {
    Token Name;
    const MacroDirective *Directive;
  };
  std::vector<FunctionMacro> Macros;
};

void MacroToFunctionCallbacks::MacroDefined(const Token &MacroNameTok,
                                            const MacroDirective *MD) {
  if (SM.getFilename(MD->getLocation()).empty())
    return;

  const MacroInfo *Info = MD->getMacroInfo();
  ArrayRef<Token> MacroTokens = Info->tokens();
  if (Info->isBuiltinMacro() || MacroTokens.empty() ||
      !Info->isFunctionLike() || Info->param_empty())
    return;

  if (llvm::any_of(Info->params(), [](const IdentifierInfo *Param) {
        return Param->getName() == "__VA_ARGS__";
      }))
    return;

  FunctionExpressionMatcher Matcher(MacroTokens, Info->params());
  if (Matcher.match())
    Macros.push_back({MacroNameTok, MD});
}

void MacroToFunctionCallbacks::MacroUndefined(const Token &MacroNameTok,
                                              const MacroDefinition &MD,
                                              const MacroDirective *Undef) {
  //
}

static StringRef getTokenName(const Token &Tok) {
  return Tok.is(tok::raw_identifier) ? Tok.getRawIdentifier()
                                     : Tok.getIdentifierInfo()->getName();
}

void MacroToFunctionCallbacks::EndOfMainFile() {
  for (const FunctionMacro &Macro : Macros) {
    Check->diag(Macro.Directive->getLocation(),
                "macro '%0' defines an expression of its arguments; "
                "prefer an inline function instead")
        << getTokenName(Macro.Name);

    const MacroInfo *Info = Macro.Directive->getMacroInfo();
    std::string Replacement = "template <";
    for (unsigned I = 0; I < Info->getNumParams(); ++I) {
      if (I > 0)
        Replacement += ", ";
      Replacement += "typename T";
      if (I > 0)
        Replacement += std::to_string(I + 1);
    }
    Replacement += "> auto " + getTokenName(Macro.Name).str() + "(";
    MacroInfo::param_iterator Param = Info->param_begin();
    for (unsigned I = 0; I < Info->getNumParams(); ++I, ++Param) {
        if (I > 0)
            Replacement += ", ";
        Replacement += "T";
        if (I > 0)
            Replacement += std::to_string(I + 1);
        Replacement += ' ' + (*Param)->getName().str();
    }
    Replacement += ") { return ";

    SourceLocation Begin = Info->getDefinitionLoc();
    Begin = SM.translateLineCol(SM.getFileID(Begin),
                                SM.getSpellingLineNumber(Begin), 1);
    CharSourceRange Range;
    Range.setBegin(Begin);
    Range.setEnd(Info->tokens_begin()->getLocation());
    SourceLocation DefinitionEnd = Lexer::getLocForEndOfToken(
        Info->getDefinitionEndLoc(), 0, SM, LangOpts);

    Check->diag(Begin, "replace macro with template function")
        << FixItHint::CreateReplacement(Range, Replacement)
        << FixItHint::CreateInsertion(DefinitionEnd, "; }");
  }
}

void MacroToFunctionCallbacks::invalidateRange(const SourceRange &Range) {
  llvm::erase_if(Macros, [Range](const FunctionMacro &Macro) {
      return Macro.Directive->getLocation() >= Range.getBegin() &&
             Macro.Directive->getLocation() <= Range.getEnd();
    });
}

void MacroToFunctionCheck::registerPPCallbacks(const SourceManager &SM,
                                               Preprocessor *PP, Preprocessor *ModuleExpanderPP) {
  auto Callback = std::make_unique<MacroToFunctionCallbacks>(this, getLangOpts(), SM);
  PPCallback = Callback.get();
  PP->addPPCallbacks(std::move(Callback));
}

void MacroToFunctionCheck::registerMatchers(MatchFinder *Finder) {
  using namespace ast_matchers;
  auto TopLevelDecl = hasParent(translationUnitDecl());
  Finder->addMatcher(decl(TopLevelDecl).bind("top"), this);
}

static bool isValid(SourceRange Range) {
  return Range.getBegin().isValid() && Range.getEnd().isValid();
}

static bool empty(SourceRange Range) {
  return Range.getBegin() == Range.getEnd();
}

void MacroToFunctionCheck::check(const MatchFinder::MatchResult &Result) {
  auto *TLDecl = Result.Nodes.getNodeAs<Decl>("top");
  if (TLDecl == nullptr)
      return;

  SourceRange Range = TLDecl->getSourceRange();
  if (auto *TemplateFn = Result.Nodes.getNodeAs<FunctionTemplateDecl>("top")) {
    if (TemplateFn->isThisDeclarationADefinition() && TemplateFn->hasBody())
      Range = SourceRange{TemplateFn->getBeginLoc(),
                          TemplateFn->getUnderlyingDecl()->getBodyRBrace()};
  }

  if (isValid(Range) && !empty(Range))
    PPCallback->invalidateRange(Range);
}

} // namespace modernize
} // namespace tidy
} // namespace clang
