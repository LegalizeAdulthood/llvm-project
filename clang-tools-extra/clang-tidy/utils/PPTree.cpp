//===-------- PPTree.cpp - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PPTree.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"
#include <memory>

namespace clang {
namespace tidy {
namespace utils {
namespace {

class PPTreeBuilderCallbacks : public PPCallbacks {
public:
  PPTreeBuilderCallbacks(PPTreeConsumer *Callback, const SourceManager &SM,
                         const LangOptions &LangOpts)
      : Callback(Callback), SM(SM), LangOpts(LangOpts) {}

  void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                          StringRef FileName, bool IsAngled,
                          CharSourceRange FilenameRange, const FileEntry *File,
                          StringRef SearchPath, StringRef RelativePath,
                          const Module *Imported,
                          SrcMgr::CharacteristicKind FileType) override;
  void EndOfMainFile() override;
  void Ident(SourceLocation Loc, StringRef Str) override;
  void PragmaDirective(SourceLocation Loc,
                       PragmaIntroducerKind Introducer) override;
  void PragmaComment(SourceLocation Loc, const IdentifierInfo *Kind,
                     StringRef Str) override;
  void PragmaMark(SourceLocation Loc, StringRef Trivia) override;
  void PragmaDetectMismatch(SourceLocation Loc, StringRef Name,
                            StringRef Value) override;
  void PragmaDebug(SourceLocation Loc, StringRef DebugType) override;
  void PragmaMessage(SourceLocation Loc, StringRef Namespace,
                     PragmaMessageKind Kind, StringRef Str) override;
  void PragmaDiagnosticPush(SourceLocation Loc, StringRef Namespace) override;
  void PragmaDiagnosticPop(SourceLocation Loc, StringRef Namespace) override;
  void PragmaDiagnostic(SourceLocation Loc, StringRef Namespace,
                        diag::Severity mapping, StringRef Str) override;
  void PragmaOpenCLExtension(SourceLocation NameLoc, const IdentifierInfo *Name,
                             SourceLocation StateLoc, unsigned State) override;
  void PragmaWarning(SourceLocation Loc, PragmaWarningSpecifier WarningSpec,
                     ArrayRef<int> Ids) override;
  void PragmaWarningPush(SourceLocation Loc, int Level) override;
  void PragmaWarningPop(SourceLocation Loc) override;
  void PragmaExecCharsetPush(SourceLocation Loc, StringRef Str) override;
  void PragmaExecCharsetPop(SourceLocation Loc) override;
  void PragmaAssumeNonNullBegin(SourceLocation Loc) override;
  void PragmaAssumeNonNullEnd(SourceLocation Loc) override;
  void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                    SourceRange Range, const MacroArgs *Args) override;
  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD,
                      const MacroDirective *Undef) override;
  void Defined(const Token &MacroNameTok, const MacroDefinition &MD,
               SourceRange Range) override;
  void HasInclude(SourceLocation Loc, StringRef FileName, bool IsAngled,
                  Optional<FileEntryRef> File,
                  SrcMgr::CharacteristicKind FileType) override;
  void SourceRangeSkipped(SourceRange Range, SourceLocation EndifLoc) override;
  void If(SourceLocation Loc, SourceRange ConditionRange,
          ConditionValueKind ConditionValue) override;
  void Elif(SourceLocation Loc, SourceRange ConditionRange,
            ConditionValueKind ConditionValue, SourceLocation IfLoc) override;
  void Ifdef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD) override;
  void Elifdef(SourceLocation Loc, const Token &MacroNameTok,
               const MacroDefinition &MD) override;
  void Elifdef(SourceLocation Loc, SourceRange ConditionRange,
               SourceLocation IfLoc) override;
  void Ifndef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD) override;
  void Elifndef(SourceLocation Loc, const Token &MacroNameTok,
                const MacroDefinition &MD) override;
  void Elifndef(SourceLocation Loc, SourceRange ConditionRange,
                SourceLocation IfLoc) override;
  void Else(SourceLocation Loc, SourceLocation IfLoc) override;
  void Endif(SourceLocation Loc, SourceLocation IfLoc) override;

private:
  PPTreeConsumer *Callback;
  const SourceManager &SM;
  const LangOptions &LangOpts;
  PPTree Tree;
};

void PPTreeBuilderCallbacks::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, const FileEntry *File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {
  auto Inclusion{std::make_unique<PPInclusion>(
      HashLoc, IncludeTok, FileName, IsAngled, FilenameRange, File, SearchPath,
      RelativePath, Imported, FileType)};
  Tree.Directives.emplace_back(Inclusion.release());
}

void PPTreeBuilderCallbacks::EndOfMainFile() { Callback->endOfMainFile(&Tree); }

void PPTreeBuilderCallbacks::Ident(SourceLocation Loc, StringRef Str) {
  auto Ident{std::make_unique<PPIdent>(Loc, Str)};
  Tree.Directives.emplace_back(Ident.release());
}

void PPTreeBuilderCallbacks::PragmaDirective(SourceLocation Loc,
                                             PragmaIntroducerKind Introducer) {
  auto Pragma{std::make_unique<PPPragma>(Loc, Introducer)};
  Tree.Directives.emplace_back(Pragma.release());
}

void PPTreeBuilderCallbacks::PragmaComment(SourceLocation Loc,
                                           const IdentifierInfo *Kind,
                                           StringRef Str) {
  auto PragmaComment{std::make_unique<PPPragmaComment>(Loc, Kind, Str)};
  Tree.Directives.emplace_back(PragmaComment.release());
}

void PPTreeBuilderCallbacks::PragmaMark(SourceLocation Loc, StringRef Trivia) {
  auto PragmaMark{std::make_unique<PPPragmaMark>(Loc, Trivia)};
  Tree.Directives.emplace_back(PragmaMark.release());
}

void PPTreeBuilderCallbacks::PragmaDetectMismatch(SourceLocation Loc,
                                                  StringRef Name,
                                                  StringRef Value) {
  auto PragmaDetectMismatch{std::make_unique<PPPragmaDetectMismatch>(Loc, Name, Value)};
  Tree.Directives.emplace_back(PragmaDetectMismatch.release());
}

void PPTreeBuilderCallbacks::PragmaDebug(SourceLocation Loc,
                                         StringRef DebugType) {
  PPCallbacks::PragmaDebug(Loc, DebugType);
}

void PPTreeBuilderCallbacks::PragmaMessage(SourceLocation Loc,
                                           StringRef Namespace,
                                           PragmaMessageKind Kind,
                                           StringRef Str) {
  PPCallbacks::PragmaMessage(Loc, Namespace, Kind, Str);
}

void PPTreeBuilderCallbacks::PragmaDiagnosticPush(SourceLocation Loc,
                                                  StringRef Namespace) {
  PPCallbacks::PragmaDiagnosticPush(Loc, Namespace);
}

void PPTreeBuilderCallbacks::PragmaDiagnosticPop(SourceLocation Loc,
                                                 StringRef Namespace) {
  PPCallbacks::PragmaDiagnosticPop(Loc, Namespace);
}

void PPTreeBuilderCallbacks::PragmaDiagnostic(SourceLocation Loc,
                                              StringRef Namespace,
                                              diag::Severity mapping,
                                              StringRef Str) {
  PPCallbacks::PragmaDiagnostic(Loc, Namespace, mapping, Str);
}

void PPTreeBuilderCallbacks::PragmaOpenCLExtension(SourceLocation NameLoc,
                                                   const IdentifierInfo *Name,
                                                   SourceLocation StateLoc,
                                                   unsigned State) {
  PPCallbacks::PragmaOpenCLExtension(NameLoc, Name, StateLoc, State);
}

void PPTreeBuilderCallbacks::PragmaWarning(SourceLocation Loc,
                                           PragmaWarningSpecifier WarningSpec,
                                           ArrayRef<int> Ids) {
  PPCallbacks::PragmaWarning(Loc, WarningSpec, Ids);
}

void PPTreeBuilderCallbacks::PragmaWarningPush(SourceLocation Loc, int Level) {
  PPCallbacks::PragmaWarningPush(Loc, Level);
}

void PPTreeBuilderCallbacks::PragmaWarningPop(SourceLocation Loc) {
  PPCallbacks::PragmaWarningPop(Loc);
}

void PPTreeBuilderCallbacks::PragmaExecCharsetPush(SourceLocation Loc,
                                                   StringRef Str) {
  PPCallbacks::PragmaExecCharsetPush(Loc, Str);
}

void PPTreeBuilderCallbacks::PragmaExecCharsetPop(SourceLocation Loc) {
  PPCallbacks::PragmaExecCharsetPop(Loc);
}

void PPTreeBuilderCallbacks::PragmaAssumeNonNullBegin(SourceLocation Loc) {
  PPCallbacks::PragmaAssumeNonNullBegin(Loc);
}

void PPTreeBuilderCallbacks::PragmaAssumeNonNullEnd(SourceLocation Loc) {
  PPCallbacks::PragmaAssumeNonNullEnd(Loc);
}

void PPTreeBuilderCallbacks::MacroExpands(const Token &MacroNameTok,
                                          const MacroDefinition &MD,
                                          SourceRange Range,
                                          const MacroArgs *Args) {
  PPCallbacks::MacroExpands(MacroNameTok, MD, Range, Args);
}

void PPTreeBuilderCallbacks::MacroDefined(const Token &MacroNameTok,
                                          const MacroDirective *MD) {
  PPCallbacks::MacroDefined(MacroNameTok, MD);
}

void PPTreeBuilderCallbacks::MacroUndefined(const Token &MacroNameTok,
                                            const MacroDefinition &MD,
                                            const MacroDirective *Undef) {
  PPCallbacks::MacroUndefined(MacroNameTok, MD, Undef);
}

void PPTreeBuilderCallbacks::Defined(const Token &MacroNameTok,
                                     const MacroDefinition &MD,
                                     SourceRange Range) {
  PPCallbacks::Defined(MacroNameTok, MD, Range);
}

void PPTreeBuilderCallbacks::HasInclude(SourceLocation Loc, StringRef FileName,
                                        bool IsAngled,
                                        Optional<FileEntryRef> File,
                                        SrcMgr::CharacteristicKind FileType) {
  PPCallbacks::HasInclude(Loc, FileName, IsAngled, File, FileType);
}

void PPTreeBuilderCallbacks::SourceRangeSkipped(SourceRange Range,
                                                SourceLocation EndifLoc) {
  PPCallbacks::SourceRangeSkipped(Range, EndifLoc);
}

void PPTreeBuilderCallbacks::If(SourceLocation Loc, SourceRange ConditionRange,
                                ConditionValueKind ConditionValue) {
  PPCallbacks::If(Loc, ConditionRange, ConditionValue);
}

void PPTreeBuilderCallbacks::Elif(SourceLocation Loc,
                                  SourceRange ConditionRange,
                                  ConditionValueKind ConditionValue,
                                  SourceLocation IfLoc) {
  PPCallbacks::Elif(Loc, ConditionRange, ConditionValue, IfLoc);
}

void PPTreeBuilderCallbacks::Ifdef(SourceLocation Loc,
                                   const Token &MacroNameTok,
                                   const MacroDefinition &MD) {
  PPCallbacks::Ifdef(Loc, MacroNameTok, MD);
}

void PPTreeBuilderCallbacks::Elifdef(SourceLocation Loc,
                                     const Token &MacroNameTok,
                                     const MacroDefinition &MD) {
  PPCallbacks::Elifdef(Loc, MacroNameTok, MD);
}

void PPTreeBuilderCallbacks::Elifdef(SourceLocation Loc,
                                     SourceRange ConditionRange,
                                     SourceLocation IfLoc) {
  PPCallbacks::Elifdef(Loc, ConditionRange, IfLoc);
}

void PPTreeBuilderCallbacks::Ifndef(SourceLocation Loc,
                                    const Token &MacroNameTok,
                                    const MacroDefinition &MD) {
  PPCallbacks::Ifndef(Loc, MacroNameTok, MD);
}

void PPTreeBuilderCallbacks::Elifndef(SourceLocation Loc,
                                      const Token &MacroNameTok,
                                      const MacroDefinition &MD) {
  PPCallbacks::Elifndef(Loc, MacroNameTok, MD);
}

void PPTreeBuilderCallbacks::Elifndef(SourceLocation Loc,
                                      SourceRange ConditionRange,
                                      SourceLocation IfLoc) {
  PPCallbacks::Elifndef(Loc, ConditionRange, IfLoc);
}

void PPTreeBuilderCallbacks::Else(SourceLocation Loc, SourceLocation IfLoc) {
  PPCallbacks::Else(Loc, IfLoc);
}

void PPTreeBuilderCallbacks::Endif(SourceLocation Loc, SourceLocation IfLoc) {
  PPCallbacks::Endif(Loc, IfLoc);
}
} // namespace

PPTreeBuilder::PPTreeBuilder(PPTreeConsumer *Callback, Preprocessor *PP,
                             const SourceManager &SM, const LangOptions &LangOpts)
    : PP(PP), Callback(Callback), SM(SM), LangOpts(LangOpts) {
  PP->addPPCallbacks(
      std::make_unique<PPTreeBuilderCallbacks>(Callback, SM, LangOpts));
}

} // namespace utils
} // namespace tidy
} // namespace clang
