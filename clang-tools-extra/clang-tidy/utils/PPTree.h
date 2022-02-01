//===---------- PPTree.h - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H

#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"
#include <string>

namespace clang {
namespace tidy {
namespace utils {

struct PPDirective {
  virtual ~PPDirective() = default;
};

struct PPInclusion : PPDirective {
  PPInclusion(SourceLocation HashLoc, Token IncludeTok, StringRef FileName,
              bool IsAngled, CharSourceRange FilenameRange,
              const FileEntry *File, StringRef SearchPath,
              StringRef RelativePath, const Module *Imported,
              SrcMgr::CharacteristicKind FileType)
      : HashLoc(HashLoc), IncludeTok(IncludeTok), FileName(FileName.str()),
        IsAngled(IsAngled), FilenameRange(FilenameRange), File(File),
        SearchPath(SearchPath.str()), RelativePath(RelativePath.str()),
        Imported(Imported), FileType(FileType) {}

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

struct PPIdent : PPDirective {
  PPIdent(SourceLocation Loc, StringRef Str) : Loc(Loc), Str(Str.str()) {}

  SourceLocation Loc;
  std::string Str;
};

struct PPPragma : PPDirective {
  PPPragma(SourceLocation Loc, PragmaIntroducerKind Introducer)
      : Loc(Loc), Introducer(Introducer) {}

  SourceLocation Loc;
  PragmaIntroducerKind Introducer;
};

struct PPPragmaComment : PPDirective {
  PPPragmaComment(SourceLocation Loc, const IdentifierInfo *Kind,
      StringRef Str) : Loc(Loc), Kind(Kind), Str(Str.str()) {}

  SourceLocation Loc;
  const IdentifierInfo *Kind;
  std::string Str;
};

struct PPPragmaMark : PPDirective {
  PPPragmaMark(SourceLocation Loc, StringRef Trivia)
      : Loc(Loc), Trivia(Trivia.str()) {}

  SourceLocation Loc;
  std::string Trivia;
};

struct PPPragmaDetectMismatch : PPDirective {
  PPPragmaDetectMismatch(SourceLocation Loc, StringRef Name, StringRef Value)
      : Loc(Loc), Name(Name.str()), Value(Value.str()) {}

  SourceLocation Loc;
  std::string Name;
  std::string Value;
};

struct PPPragmaDebug : PPDirective {
  PPPragmaDebug(SourceLocation Loc, StringRef DebugType)
      : Loc(Loc), DebugType(DebugType.str()) {}

  SourceLocation Loc;
  std::string DebugType;
};

struct PPPragmaMessage : PPDirective {
  PPPragmaMessage(SourceLocation Loc, StringRef Namespace,
                  PPCallbacks::PragmaMessageKind Kind, StringRef Str)
      : Loc(Loc), Namespace(Namespace.str()), Kind(Kind), Str(Str.str()) {}

  SourceLocation Loc;
  std::string Namespace;
  PPCallbacks::PragmaMessageKind Kind;
  std::string Str;
};

struct PPTree {
  std::vector<PPDirective *> Directives;
};

class PPTreeConsumer {
public:
  virtual ~PPTreeConsumer() = default;

  virtual void endOfMainFile(const utils::PPTree *Tree) = 0;
};

class PPTreeBuilder {
public:
  PPTreeBuilder(PPTreeConsumer *Callback, Preprocessor *PP, const SourceManager &SM, const LangOptions &LangOpts);

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
