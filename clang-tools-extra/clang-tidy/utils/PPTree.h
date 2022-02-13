//===---------- PPTree.h - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"
#include <string>

namespace clang {
namespace tidy {
namespace utils {

class PPDirective {
public:
  enum DirectiveKind {
    DK_Inclusion,
    DK_Ident,
    DK_Pragma,
    DK_PragmaComment,
    DK_PragmaDebug,
    DK_PragmaDetectMismatch,
    DK_PragmaMark,
    DK_PragmaMessage,
    DK_MacroDefined,
    DK_MacroUndefined,
    DK_If,
    DK_Else,
    DK_ElseIf,
    DK_IfDef,
    DK_IfNotDef,
    DK_ElseIfDef,
    DK_ElseIfNotDef,
    DK_EndIf,
  };

  virtual ~PPDirective() = default;

  DirectiveKind getKind() const { return Kind; }

protected:
  PPDirective(DirectiveKind Kind) : Kind(Kind) {}

private:
  DirectiveKind Kind;
};

using PPDirectiveStorage = std::vector<PPDirective *>;

class PPDirectiveList {
public:
  void add(PPDirective *Dir) { Directives.emplace_back(Dir); }

  ~PPDirectiveList() {
    for (PPDirective *Dir : Directives)
      delete Dir;
    Directives.clear();
  }

  PPDirectiveStorage::iterator begin() { return Directives.begin(); }
  PPDirectiveStorage::iterator end() { return Directives.end(); }

  PPDirectiveStorage::const_iterator begin() const {
    return Directives.begin();
  }
  PPDirectiveStorage::const_iterator end() const { return Directives.end(); }

  size_t size() const { return Directives.size(); }

private:
  PPDirectiveStorage Directives;
};

class PPInclusion : public PPDirective {
public:
  PPInclusion(SourceLocation HashLoc, Token IncludeTok, StringRef FileName,
              bool IsAngled, CharSourceRange FilenameRange,
              const FileEntry *File, StringRef SearchPath,
              StringRef RelativePath, const Module *Imported,
              SrcMgr::CharacteristicKind FileType)
      : PPDirective(DK_Inclusion), HashLoc(HashLoc), IncludeTok(IncludeTok),
        FileName(FileName.str()), IsAngled(IsAngled),
        FilenameRange(FilenameRange), File(File), SearchPath(SearchPath.str()),
        RelativePath(RelativePath.str()), Imported(Imported),
        FileType(FileType) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_Inclusion;
  }

  SourceLocation HashLoc;
  Token IncludeTok;
  std::string FileName;
  bool IsAngled;
  CharSourceRange FilenameRange;
  const FileEntry *File;
  std::string SearchPath;
  std::string RelativePath;
  const Module *Imported;
  SrcMgr::CharacteristicKind FileType;
};

class PPIdent : public PPDirective {
public:
  PPIdent(SourceLocation Loc, StringRef Str)
      : PPDirective(DK_Ident), Loc(Loc), Str(Str.str()) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_Ident; }

  SourceLocation Loc;
  std::string Str;
};

class PPPragma : public PPDirective {
public:
  PPPragma(SourceLocation Loc, PragmaIntroducerKind Introducer)
      : PPDirective(DK_Pragma), Loc(Loc), Introducer(Introducer) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_Pragma;
  }

  SourceLocation Loc;
  PragmaIntroducerKind Introducer;
};

class PPPragmaComment : public PPDirective {
public:
  PPPragmaComment(SourceLocation Loc, const IdentifierInfo *Kind, StringRef Str)
      : PPDirective(DK_PragmaComment), Loc(Loc), Kind(Kind), Str(Str.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaComment;
  }

  SourceLocation Loc;
  const IdentifierInfo *Kind;
  std::string Str;
};

class PPPragmaDebug : public PPDirective {
public:
  PPPragmaDebug(SourceLocation Loc, StringRef DebugType)
      : PPDirective(DK_PragmaDebug), Loc(Loc), DebugType(DebugType.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaDebug;
  }

  SourceLocation Loc;
  std::string DebugType;
};

class PPPragmaDetectMismatch : public PPDirective {
public:
  PPPragmaDetectMismatch(SourceLocation Loc, StringRef Name, StringRef Value)
      : PPDirective(DK_PragmaDetectMismatch), Loc(Loc), Name(Name.str()),
        Value(Value.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaDetectMismatch;
  }

  SourceLocation Loc;
  std::string Name;
  std::string Value;
};

class PPPragmaMark : public PPDirective {
public:
  PPPragmaMark(SourceLocation Loc, StringRef Trivia)
      : PPDirective(DK_PragmaMark), Loc(Loc), Trivia(Trivia.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaMark;
  }

  SourceLocation Loc;
  std::string Trivia;
};

class PPPragmaMessage : public PPDirective {
public:
  PPPragmaMessage(SourceLocation Loc, StringRef Namespace,
                  PPCallbacks::PragmaMessageKind Kind, StringRef Str)
      : PPDirective(DK_PragmaMessage), Loc(Loc), Namespace(Namespace.str()),
        Kind(Kind), Str(Str.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaMessage;
  }

  SourceLocation Loc;
  std::string Namespace;
  PPCallbacks::PragmaMessageKind Kind;
  std::string Str;
};

class PPMacroDefined : public PPDirective {
public:
  PPMacroDefined(const Token &MacroNameTok, const MacroDirective *MD)
      : PPDirective(DK_MacroDefined), Name(MacroNameTok), MD(MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_MacroDefined;
  }

  Token Name;
  const MacroDirective *MD;
};

class PPMacroUndefined : public PPDirective {
public:
  PPMacroUndefined(Token Name, const MacroDefinition *MD,
                   const MacroDirective *Undef)
      : PPDirective(DK_MacroUndefined), Name(Name), MD(MD), Undef(Undef) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_MacroUndefined;
  }

  Token Name;
  const MacroDefinition *MD;
  const MacroDirective *Undef;
};

class PPIf : public PPDirective {
public:
  PPIf(SourceLocation Loc, SourceRange ConditionRange,
       PPCallbacks::ConditionValueKind ConditionValue)
      : PPDirective(DK_If), Loc(Loc), ConditionRange(ConditionRange),
        ConditionValue(ConditionValue) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_If; }

  SourceLocation Loc;
  SourceRange ConditionRange;
  PPCallbacks::ConditionValueKind ConditionValue;
  PPDirectiveList Directives;
};

class PPElse : public PPDirective {
public:
  PPElse(SourceLocation Loc, SourceLocation IfLoc)
      : PPDirective(DK_Else), Loc(Loc), IfLoc(IfLoc) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_Else; }

  SourceLocation Loc;
  SourceLocation IfLoc;
  PPDirectiveList Directives;
};

class PPElseIf : public PPDirective {
public:
  PPElseIf(SourceLocation Loc, SourceRange ConditionRange,
           PPCallbacks::ConditionValueKind ConditionValue, SourceLocation IfLoc)
      : PPDirective(DK_ElseIf), Loc(Loc), ConditionRange(ConditionRange),
        ConditionValue(ConditionValue), IfLoc(IfLoc) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_ElseIf;
  }

  SourceLocation Loc;
  SourceRange ConditionRange;
  PPCallbacks::ConditionValueKind ConditionValue;
  SourceLocation IfLoc;
  PPDirectiveList Directives;
};

class PPIfDef : public PPDirective {
public:
  PPIfDef(SourceLocation Loc, const Token &MacroNameTok,
          const MacroDefinition &MD)
      : PPDirective(DK_IfDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_IfDef; }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPIfNotDef : public PPDirective {
public:
  PPIfNotDef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD)
      : PPDirective(DK_IfNotDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_IfNotDef;
  }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPElseIfDef : public PPDirective {
public:
  PPElseIfDef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD)
      : PPDirective(DK_ElseIfDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_ElseIfDef;
  }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPElseIfNotDef : public PPDirective {
public:
  PPElseIfNotDef(SourceLocation Loc, const Token &MacroNameTok,
                 const MacroDefinition &MD)
      : PPDirective(DK_ElseIfNotDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_ElseIfNotDef;
  }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPEndIf : public PPDirective {
public:
  PPEndIf(SourceLocation Loc, SourceLocation IfLoc)
      : PPDirective(DK_EndIf), Loc(Loc), IfLoc(IfLoc) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_EndIf; }

  SourceLocation Loc;
  SourceLocation IfLoc;
};

struct PPTree {
  PPDirectiveList Directives;
};

class PPTreeConsumer {
public:
  virtual ~PPTreeConsumer() = default;

  virtual void endOfMainFile(const utils::PPTree *Tree) = 0;
};

class PPTreeBuilder {
public:
  PPTreeBuilder(PPTreeConsumer *Callback, Preprocessor *PP,
                const SourceManager &SM, const LangOptions &LangOpts);

private:
  Preprocessor *PP;
  PPTreeConsumer *Callback;
  const SourceManager &SM;
  const LangOptions &LangOpts;
};

} // namespace utils
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H
