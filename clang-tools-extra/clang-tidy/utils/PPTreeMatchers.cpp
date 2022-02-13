//===-------- PPTree.cpp - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PPTreeMatchers.h"
#include "PPTreeVisitor.h"
#include "llvm/ADT/Optional.h"

namespace clang {
namespace tidy {
namespace utils {

const PPMatcher<PPInclusion> ppInclusion;
const PPMatcher<PPIdent> identDirective;
const PPMatcher<PPPragma> pragmaDirective;
const PPMatcher<PPPragmaComment> pragmaCommentDirective;
const PPMatcher<PPPragmaMark> pragmaMarkDirective;
const PPMatcher<PPPragmaDetectMismatch> pragmaDetectMismatchDirective;
const PPMatcher<PPPragmaDebug> pragmaDebugDirective;
const PPMatcher<PPPragmaMessage> pragmaMessage;
const PPMatcher<PPMacroDefined> macroDefinedDirective;
const PPMatcher<PPMacroUndefined> macroUndefinedDirective;
const PPMatcher<PPIf> ifDirective;
const PPMatcher<PPElse> elseDirective;
const PPMatcher<PPElseIf> elseIfDirective;
const PPMatcher<PPIfDef> ifDefDirective;
const PPMatcher<PPIfNotDef> ifNotDefDirective;
const PPMatcher<PPElseIfDef> elseIfDefDirective;
const PPMatcher<PPElseIfNotDef> elseIfNotDefDirective;
const PPMatcher<PPEndIf> endIfDirective;

namespace {

template <typename T> class ValueMatcher {
public:
  ValueMatcher(T Value) : Value(Value) {}

  bool match(T Actual) const { return Actual == Value; }

private:
  T Value;
};

using BoolMatcher = ValueMatcher<bool>;
using CharacteristicKindMatcher = ValueMatcher<SrcMgr::CharacteristicKind>;

class StringRefMatcher {
public:
  StringRefMatcher(const std::string &Value) : Value(Value) {}

  bool match(StringRef Text) const { return Text.str() == Value; }

private:
  const std::string &Value;
};

class SourceLocationMatcher {
public:
  SourceLocationMatcher(SourceLocation Loc) : Loc(Loc) {}

  bool match(SourceLocation Value) const;

private:
  SourceLocation Loc;
};

bool SourceLocationMatcher::match(SourceLocation Value) const {
  return Value == Loc;
}

class TokenMatcher {
public:
  TokenMatcher(tok::TokenKind Kind) : Kind(Kind) {}

  bool match(const Token &Tok) const;

private:
  tok::TokenKind Kind;
};

bool TokenMatcher::match(const Token &Tok) const { return Tok.is(Kind); }

class PPInclusionMatcher {
public:
  PPInclusionMatcher(Optional<SourceLocationMatcher> HashLoc,
                     Optional<TokenMatcher> IncludeTok,
                     Optional<StringRefMatcher> FileName,
                     Optional<BoolMatcher> IsAngled,
                     Optional<StringRefMatcher> SearchPath,
                     Optional<StringRefMatcher> RelativePath,
                     Optional<CharacteristicKindMatcher> FileType)
      : HashLocMatcher(HashLoc), IncludeTokMatcher(IncludeTok),
        FileNameMatcher(FileName), IsAngledMatcher(IsAngled),
        SearchPathMatcher(SearchPath), RelativePathMatcher(RelativePath),
        FileTypeMatcher(FileType),
        HasSubMatchers(HashLoc || IncludeTok || FileName || IsAngled ||
                       SearchPath || RelativePath || FileType) {}

  bool match(const PPInclusion *Directive) const;

private:
  Optional<SourceLocationMatcher> HashLocMatcher;
  Optional<TokenMatcher> IncludeTokMatcher;
  Optional<StringRefMatcher> FileNameMatcher;
  Optional<BoolMatcher> IsAngledMatcher;
  Optional<StringRefMatcher> SearchPathMatcher;
  Optional<StringRefMatcher> RelativePathMatcher;
  Optional<CharacteristicKindMatcher> FileTypeMatcher;
  bool HasSubMatchers;
};

bool PPInclusionMatcher::match(const PPInclusion *Directive) const {
  if (!HasSubMatchers)
    return false;

  bool M = true;
  if (HashLocMatcher.hasValue())
    M = M && HashLocMatcher.getValue().match(Directive->HashLoc);
  if (M && IncludeTokMatcher.hasValue())
    M = M && IncludeTokMatcher.getValue().match(Directive->IncludeTok);
  if (M && FileNameMatcher.hasValue())
    M = M && FileNameMatcher.getValue().match(Directive->FileName);
  if (M && IsAngledMatcher.hasValue())
    M = M && IsAngledMatcher.getValue().match(Directive->IsAngled);
  if (M && SearchPathMatcher.hasValue())
    M = M && SearchPathMatcher.getValue().match(Directive->SearchPath);
  if (M && RelativePathMatcher.hasValue())
    M = M && RelativePathMatcher.getValue().match(Directive->RelativePath);
  if (M && FileTypeMatcher.hasValue())
    M = M && FileTypeMatcher.getValue().match(Directive->FileType);

  return M;
}

class PPIdentMatcher {
public:
  PPIdentMatcher(Optional<SourceLocationMatcher> Loc,
                 Optional<StringRefMatcher> Str)
      : LocMatcher(Loc), StrMatcher(Str), HasSubMatchers(Loc || Str) {}

  bool match(const PPIdent *Directive) const;

private:
  Optional<SourceLocationMatcher> LocMatcher;
  Optional<StringRefMatcher> StrMatcher;
  bool HasSubMatchers;
};

bool PPIdentMatcher::match(const PPIdent *Directive) const {
  if (!HasSubMatchers)
    return false;

  bool M = true;
  if (LocMatcher.hasValue())
    M = M && LocMatcher.getValue().match(Directive->Loc);
  if (StrMatcher.hasValue())
    M = M && StrMatcher.getValue().match(Directive->Str);

  return M;
}

template <typename T> ValueMatcher<T> hasValue(T Value) {
  return ValueMatcher<T>(Value);
}

StringRefMatcher hasStringRef(const char *Value) {
  return StringRefMatcher(Value);
}

SourceLocationMatcher hasLocation(SourceLocation Loc) {
  return SourceLocationMatcher(Loc);
}

class MatchPPTreeVisitor : public PPTreeVisitor<MatchPPTreeVisitor> {
public:
  void addMatcher(PPInclusionMatcher &&Matcher) {
    InclusionMatchers.emplace_back(std::move(Matcher));
  }
  void addMatcher(PPIdentMatcher &&Matcher) {
    IdentMatchers.emplace_back(std::move(Matcher));
  }

  bool visitInclusion(const PPInclusion *Directive);
  bool visitIdent(const PPIdent *Directive);
  bool visitPragma(const PPPragma *Directive);
  bool visitPragmaComment(const PPPragmaComment *Directive);
  bool visitPragmaDebug(const PPPragmaDebug *Directive);
  bool visitPragmaDetectMismatch(const PPPragmaDetectMismatch *Directive);
  bool visitPragmaMark(const PPPragmaMark *Directive);
  bool visitPragmaMessage(const PPPragmaMessage *Directive);
  bool visitMacroDefined(const PPMacroDefined *Directive);
  bool visitMacroUndefined(const PPMacroUndefined *Directive);
  bool visitIf(const PPIf *Directive);
  bool visitElse(const PPElse *Directive);
  bool visitElseIf(const PPElseIf *Directive);
  bool visitIfDef(const PPIfDef *Directive);
  bool visitIfNotDef(const PPIfNotDef *Directive);
  bool visitElseIfDef(const PPElseIfDef *Directive);
  bool visitElseIfNotDef(const PPElseIfNotDef *Directive);
  bool visitEndIf(const PPEndIf *Directive);

private:
  std::vector<PPInclusionMatcher> InclusionMatchers;
  std::vector<PPIdentMatcher> IdentMatchers;
};

bool MatchPPTreeVisitor::visitInclusion(const PPInclusion *Directive) {
  return llvm::none_of(InclusionMatchers,
                       [Directive](const PPInclusionMatcher &Matcher) {
                         return Matcher.match(Directive);
                       });
}

bool MatchPPTreeVisitor::visitIdent(const PPIdent *Directive) {
  return llvm::none_of(IdentMatchers,
                       [Directive](const PPIdentMatcher &Matcher) {
                         return Matcher.match(Directive);
                       });
}

bool MatchPPTreeVisitor::visitPragma(const PPPragma *Directive) { return true; }

bool MatchPPTreeVisitor::visitPragmaComment(const PPPragmaComment *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitPragmaDebug(const PPPragmaDebug *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitPragmaDetectMismatch(
    const PPPragmaDetectMismatch *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitPragmaMark(const PPPragmaMark *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitPragmaMessage(const PPPragmaMessage *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitMacroDefined(const PPMacroDefined *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitMacroUndefined(
    const PPMacroUndefined *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitIf(const PPIf *Directive) { return true; }

bool MatchPPTreeVisitor::visitElse(const PPElse *Directive) { return true; }

bool MatchPPTreeVisitor::visitElseIf(const PPElseIf *Directive) { return true; }

bool MatchPPTreeVisitor::visitIfDef(const PPIfDef *Directive) { return true; }

bool MatchPPTreeVisitor::visitIfNotDef(const PPIfNotDef *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitElseIfDef(const PPElseIfDef *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitElseIfNotDef(const PPElseIfNotDef *Directive) {
  return true;
}

bool MatchPPTreeVisitor::visitEndIf(const PPEndIf *Directive) { return true; }

} // namespace

void DirectiveMatchFinder::match(const PPTree *Tree) {
  MatchPPTreeVisitor Visitor;
  {
    Optional<SourceLocationMatcher> HashLoc;
    Optional<TokenMatcher> IncludeTok;
    Optional<StringRefMatcher> FileName;
    Optional<BoolMatcher> IsAngled(BoolMatcher(true));
    Optional<StringRefMatcher> SearchPath;
    Optional<StringRefMatcher> RelativePath;
    Optional<CharacteristicKindMatcher> CharacteristicKind;
    Visitor.addMatcher(PPInclusionMatcher(HashLoc, IncludeTok, FileName,
                                          IsAngled, SearchPath, RelativePath,
                                          CharacteristicKind));
  }
  {
    Optional<SourceLocationMatcher> Loc;
    Optional<StringRefMatcher> Str;
    Visitor.addMatcher(PPIdentMatcher(Loc, Str));
  }

  Visitor.visit(Tree);
}

} // namespace utils
} // namespace tidy
} // namespace clang
