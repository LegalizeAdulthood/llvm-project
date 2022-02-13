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

  void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                    SourceRange Range, const MacroArgs *Args) override;
  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD,
                      const MacroDirective *Undef) override;
  void Defined(const Token &MacroNameTok, const MacroDefinition &MD,
               SourceRange Range) override;

  void If(SourceLocation Loc, SourceRange ConditionRange,
          ConditionValueKind ConditionValue) override;
  void Ifdef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD) override;
  void Ifndef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD) override;
  void Else(SourceLocation Loc, SourceLocation IfLoc) override;
  void Elif(SourceLocation Loc, SourceRange ConditionRange,
            ConditionValueKind ConditionValue, SourceLocation IfLoc) override;
  void Elifdef(SourceLocation Loc, const Token &MacroNameTok,
               const MacroDefinition &MD) override;
  void Elifndef(SourceLocation Loc, const Token &MacroNameTok,
                const MacroDefinition &MD) override;
  void Endif(SourceLocation Loc, SourceLocation IfLoc) override;
  void pushDirectiveStack(PPDirectiveList *List);

private:
  void popDirectiveStack();

  PPTreeConsumer *Callback;
  const SourceManager &SM;
  const LangOptions &LangOpts;
  PPTree Tree;
  std::vector<PPDirectiveList *> DirectiveStack{&Tree.Directives};
  PPDirectiveList *Directives = DirectiveStack.back();
};

void PPTreeBuilderCallbacks::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, const FileEntry *File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {
  auto Directive{std::make_unique<PPInclusion>(
      HashLoc, IncludeTok, FileName, IsAngled, FilenameRange, File, SearchPath,
      RelativePath, Imported, FileType)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::EndOfMainFile() { Callback->endOfMainFile(&Tree); }

void PPTreeBuilderCallbacks::Ident(SourceLocation Loc, StringRef Str) {
  auto Directive{std::make_unique<PPIdent>(Loc, Str)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::PragmaDirective(SourceLocation Loc,
                                             PragmaIntroducerKind Introducer) {
  auto Directive{std::make_unique<PPPragma>(Loc, Introducer)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::PragmaComment(SourceLocation Loc,
                                           const IdentifierInfo *Kind,
                                           StringRef Str) {
  auto Directive{std::make_unique<PPPragmaComment>(Loc, Kind, Str)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::PragmaMark(SourceLocation Loc, StringRef Trivia) {
  auto Directive{std::make_unique<PPPragmaMark>(Loc, Trivia)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::PragmaDetectMismatch(SourceLocation Loc,
                                                  StringRef Name,
                                                  StringRef Value) {
  auto Directive{std::make_unique<PPPragmaDetectMismatch>(Loc, Name, Value)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::PragmaDebug(SourceLocation Loc,
                                         StringRef DebugType) {
  auto Directive{std::make_unique<PPPragmaDebug>(Loc, DebugType)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::PragmaMessage(SourceLocation Loc,
                                           StringRef Namespace,
                                           PragmaMessageKind Kind,
                                           StringRef Str) {
  auto Directive{std::make_unique<PPPragmaMessage>(Loc, Namespace, Kind, Str)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::MacroExpands(const Token &MacroNameTok,
                                          const MacroDefinition &MD,
                                          SourceRange Range,
                                          const MacroArgs *Args) {}

void PPTreeBuilderCallbacks::MacroDefined(const Token &MacroNameTok,
                                          const MacroDirective *MD) {
  auto Directive{std::make_unique<PPMacroDefined>(MacroNameTok, MD)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::MacroUndefined(const Token &MacroNameTok,
                                            const MacroDefinition &MD,
                                            const MacroDirective *Undef) {
  auto Directive{std::make_unique<PPMacroUndefined>(MacroNameTok, &MD, Undef)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::Defined(const Token &MacroNameTok,
                                     const MacroDefinition &MD,
                                     SourceRange Range) {}

void PPTreeBuilderCallbacks::If(SourceLocation Loc, SourceRange ConditionRange,
                                ConditionValueKind ConditionValue) {
  auto Directive{std::make_unique<PPIf>(Loc, ConditionRange, ConditionValue)};
  PPDirectiveList *NewContext = &Directive->Directives;
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Ifdef(SourceLocation Loc,
                                   const Token &MacroNameTok,
                                   const MacroDefinition &MD) {
  auto Directive{std::make_unique<PPIfDef>(Loc, MacroNameTok, MD)};
  PPDirectiveList *NewContext = &Directive->Directives;
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Ifndef(SourceLocation Loc,
                                    const Token &MacroNameTok,
                                    const MacroDefinition &MD) {
  auto Directive{std::make_unique<PPIfNotDef>(Loc, MacroNameTok, MD)};
  PPDirectiveList *NewContext = &Directive->Directives;
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Else(SourceLocation Loc, SourceLocation IfLoc) {
  auto Directive{std::make_unique<PPElse>(Loc, IfLoc)};
  PPDirectiveList *NewContext = &Directive->Directives;
  popDirectiveStack();
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Elif(SourceLocation Loc,
                                  SourceRange ConditionRange,
                                  ConditionValueKind ConditionValue,
                                  SourceLocation IfLoc) {
  auto Directive{
      std::make_unique<PPElseIf>(Loc, ConditionRange, ConditionValue, IfLoc)};
  PPDirectiveList *NewContext = &Directive->Directives;
  popDirectiveStack();
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Elifdef(SourceLocation Loc,
                                     const Token &MacroNameTok,
                                     const MacroDefinition &MD) {
  auto Directive{std::make_unique<PPElseIfDef>(Loc, MacroNameTok, MD)};
  PPDirectiveList *NewContext = &Directive->Directives;
  popDirectiveStack();
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Elifndef(SourceLocation Loc,
                                      const Token &MacroNameTok,
                                      const MacroDefinition &MD) {
  auto Directive{std::make_unique<PPElseIfNotDef>(Loc, MacroNameTok, MD)};
  PPDirectiveList *NewContext = &Directive->Directives;
  popDirectiveStack();
  Directives->add(Directive.release());
  pushDirectiveStack(NewContext);
}

void PPTreeBuilderCallbacks::Endif(SourceLocation Loc, SourceLocation IfLoc) {
  popDirectiveStack();
  auto Directive{std::make_unique<PPEndIf>(Loc, IfLoc)};
  Directives->add(Directive.release());
}

void PPTreeBuilderCallbacks::pushDirectiveStack(PPDirectiveList *List) {
  DirectiveStack.push_back(List);
  Directives = DirectiveStack.back();
}

void PPTreeBuilderCallbacks::popDirectiveStack() {
  assert(DirectiveStack.size() > 1);
  DirectiveStack.pop_back();
  Directives = DirectiveStack.back();
}

} // namespace

PPTreeBuilder::PPTreeBuilder(PPTreeConsumer *Callback, Preprocessor *PP,
                             const SourceManager &SM,
                             const LangOptions &LangOpts)
    : PP(PP), Callback(Callback), SM(SM), LangOpts(LangOpts) {
  PP->addPPCallbacks(
      std::make_unique<PPTreeBuilderCallbacks>(Callback, SM, LangOpts));
}

} // namespace utils
} // namespace tidy
} // namespace clang
