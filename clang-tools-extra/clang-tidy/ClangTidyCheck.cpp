//===--- ClangTidyCheck.cpp - clang-tidy ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ClangTidyCheck.h"
#include "utils/PPTree.h"
#include "clang/Lex/MacroInfo.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/YAMLParser.h"

#include <iostream>
#include <optional>

namespace clang::tidy {

using namespace utils;

static size_t IndentLevel = 0;

static std::string printToString(const SourceManager &SM, CharSourceRange R) {
  return R.getBegin().printToString(SM) + ", " + R.getEnd().printToString(SM);
}

static std::string indent() { return std::string(IndentLevel * 2, '.'); }

static auto &errs() { return llvm::errs() << indent(); }

static void dumpDirectives(const SourceManager &SM,
                           const PPDirectiveList &Directives);

static void dumpInclusion(const SourceManager &SM, const PPInclusion *D) {
  errs() << "Inclusion\n"
         << indent() << D->HashLoc.printToString(SM) << '\n'
         << indent() << D->IncludeTok.getIdentifierInfo()->getName().str()
         << '\n'
         << indent() << D->FileName << '\n'
         << indent() << (D->IsAngled ? "Angled\n" : "") << indent()
         << printToString(SM, D->FilenameRange) << '\n'
         << indent() << D->File->getDir()->getName().str() << '\n'
         << indent() << D->SearchPath << '\n'
         << indent() << D->RelativePath << '\n'
         << indent() << (D->Imported != nullptr ? "<Imported>\n" : "")
         << indent() << "FileType " << indent() << D->FileType << '\n';
}
static void dumpIdent(const SourceManager &SM, const PPIdent *D) {
  errs() << "Ident\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Str << '\n';
}
static void dumpPragma(const SourceManager &SM, const PPPragma *D) {
  errs() << "Pragma\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << "Introducer " << D->Introducer << '\n';
}
static void dumpPragmaComment(const SourceManager &SM,
                              const PPPragmaComment *D) {
  errs() << "Comment\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Kind->getName().str() << '\n'
         << indent() << D->Str << '\n';
}
static void dumpPragmaMark(const SourceManager &SM, const PPPragmaMark *D) {
  errs() << "Mark\n"
         << D->Loc.printToString(SM) << '\n'
         << indent() << D->Trivia << '\n';
}
static void dumpPragmaDetectMismatch(const SourceManager &SM,
                                     const PPPragmaDetectMismatch *D) {
  errs() << "Detect Mismatch\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name << '\n'
         << indent() << D->Value << '\n';
}
static void dumpPragmaDebug(const SourceManager &SM, const PPPragmaDebug *D) {
  errs() << "Debug\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->DebugType << '\n';
}
static void dumpPragmaMessage(const SourceManager &SM,
                              const PPPragmaMessage *D) {
  errs() << "Message\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Namespace << '\n'
         << indent() << D->Kind << '\n'
         << indent() << D->Str << '\n';
}
static void dumpMacroDefined(const SourceManager &SM, const PPMacroDefined *D) {
  errs() << "Macro Defined\n"
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
}
static void dumpMacroUndefined(const SourceManager &SM,
                               const PPMacroUndefined *D) {
  errs() << "Macro Undefined\n"
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
}
static void dumpIf(const SourceManager &SM, const PPIf *D) {
  errs() << "If\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->ConditionRange.getBegin().printToString(SM) << ", "
         << D->ConditionRange.getEnd().printToString(SM) << '\n'
         << indent() << D->ConditionValue << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpElse(const SourceManager &SM, const PPElse *D) {
  errs() << "Else\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->IfLoc.printToString(SM) << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpElseIf(const SourceManager &SM, const PPElseIf *D) {
  errs() << "ElseIf\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->ConditionRange.getBegin().printToString(SM) << ", "
         << D->ConditionRange.getEnd().printToString(SM) << '\n'
         << indent() << D->ConditionValue << '\n'
         << indent() << D->IfLoc.printToString(SM) << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpIfDef(const SourceManager &SM, const PPIfDef *D) {
  errs() << "IfDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpIfNotDef(const SourceManager &SM, const PPIfNotDef *D) {
  errs() << "IfNotDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpElseIfDef(const SourceManager &SM, const PPElseIfDef *D) {
  errs() << "ElseIfDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpElseIfNotDef(const SourceManager &SM, const PPElseIfNotDef *D) {
  errs() << "ElseIfNotDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  ++IndentLevel;
  dumpDirectives(SM, D->Directives);
  --IndentLevel;
}
static void dumpEndIf(const SourceManager &SM, const PPEndIf *D) {
  errs() << "EndIf\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->IfLoc.printToString(SM) << '\n';
}

static void dumpDirectives(const SourceManager &SM,
                           const PPDirectiveList &Directives) {
  for (const PPDirective *Directive : Directives) {
    if (const PPMacroDefined *Def = dyn_cast<PPMacroDefined>(Directive))
      dumpMacroDefined(SM, Def);
    else if (const PPInclusion *I = dyn_cast<PPInclusion>(Directive))
      dumpInclusion(SM, I);
    else if (const PPIdent *ID = dyn_cast<PPIdent>(Directive))
      dumpIdent(SM, ID);
    else if (const PPPragma *P = dyn_cast<PPPragma>(Directive))
      dumpPragma(SM, P);
    else if (const PPPragmaComment *C = dyn_cast<PPPragmaComment>(Directive))
      dumpPragmaComment(SM, C);
    else if (const PPPragmaMark *M = dyn_cast<PPPragmaMark>(Directive))
      dumpPragmaMark(SM, M);
    else if (const PPPragmaDetectMismatch *MM =
                 dyn_cast<PPPragmaDetectMismatch>(Directive))
      dumpPragmaDetectMismatch(SM, MM);
    else if (const PPPragmaDebug *Dbg = dyn_cast<PPPragmaDebug>(Directive))
      dumpPragmaDebug(SM, Dbg);
    else if (const PPPragmaMessage *Msg = dyn_cast<PPPragmaMessage>(Directive))
      dumpPragmaMessage(SM, Msg);
    else if (const PPMacroUndefined *Undef =
                 dyn_cast<PPMacroUndefined>(Directive))
      dumpMacroUndefined(SM, Undef);
    else if (const PPIf *If = dyn_cast<PPIf>(Directive))
      dumpIf(SM, If);
    else if (const PPElse *Else = dyn_cast<PPElse>(Directive))
      dumpElse(SM, Else);
    else if (const PPElseIf *ElseIf = dyn_cast<PPElseIf>(Directive))
      dumpElseIf(SM, ElseIf);
    else if (const PPIfDef *IfDef = dyn_cast<PPIfDef>(Directive))
      dumpIfDef(SM, IfDef);
    else if (const PPIfNotDef *IfNotDef = dyn_cast<PPIfNotDef>(Directive))
      dumpIfNotDef(SM, IfNotDef);
    else if (const PPElseIfDef *ElseIfDef = dyn_cast<PPElseIfDef>(Directive))
      dumpElseIfDef(SM, ElseIfDef);
    else if (const PPElseIfNotDef *ElseIfNotDef =
                 dyn_cast<PPElseIfNotDef>(Directive))
      dumpElseIfNotDef(SM, ElseIfNotDef);
    else if (const PPEndIf *EndIf = dyn_cast<PPEndIf>(Directive))
      dumpEndIf(SM, EndIf);
  }
}

namespace {

class CheckPPTreeConsumer : public PPTreeConsumer {
public:
  CheckPPTreeConsumer(ClangTidyCheck *Check) : Check(Check) {}
  void endOfMainFile(const PPTree *Tree) override {
    llvm::errs() << "End of main file: " << Tree->Directives.size()
                 << " directives.\n";
    dumpDirectives(*SM, Tree->Directives);
  }

  ClangTidyCheck *Check;
  const SourceManager *SM{};
};

} // namespace

static std::unique_ptr<CheckPPTreeConsumer> TreeConsumer;
static std::unique_ptr<PPTreeBuilder> TreeBuilder;

ClangTidyCheck::ClangTidyCheck(StringRef CheckName, ClangTidyContext *Context)
    : CheckName(CheckName), Context(Context),
      Options(CheckName, Context->getOptions().CheckOptions, Context) {
  assert(Context != nullptr);
  assert(!CheckName.empty());
}

void ClangTidyCheck::registerPPCallbacks(const SourceManager &SM,
                                         Preprocessor *PP,
                                         Preprocessor *ModuleExpanderPP) {
  TreeConsumer.reset(new CheckPPTreeConsumer(this));
  TreeBuilder.reset(
      new PPTreeBuilder(TreeConsumer.get(), PP, SM, getLangOpts()));
}

DiagnosticBuilder ClangTidyCheck::diag(SourceLocation Loc,
                                       StringRef Description,
                                       DiagnosticIDs::Level Level) {
  return Context->diag(CheckName, Loc, Description, Level);
}

DiagnosticBuilder ClangTidyCheck::diag(StringRef Description,
                                       DiagnosticIDs::Level Level) {
  return Context->diag(CheckName, Description, Level);
}

DiagnosticBuilder
ClangTidyCheck::configurationDiag(StringRef Description,
                                  DiagnosticIDs::Level Level) const {
  return Context->configurationDiag(Description, Level);
}

void ClangTidyCheck::run(const ast_matchers::MatchFinder::MatchResult &Result) {
  // For historical reasons, checks don't implement the MatchFinder run()
  // callback directly. We keep the run()/check() distinction to avoid interface
  // churn, and to allow us to add cross-cutting logic in the future.
  check(Result);
}

ClangTidyCheck::OptionsView::OptionsView(
    StringRef CheckName, const ClangTidyOptions::OptionMap &CheckOptions,
    ClangTidyContext *Context)
    : NamePrefix((CheckName + ".").str()), CheckOptions(CheckOptions),
      Context(Context) {}

std::optional<StringRef>
ClangTidyCheck::OptionsView::get(StringRef LocalName) const {
  if (Context->getOptionsCollector())
    Context->getOptionsCollector()->insert((NamePrefix + LocalName).str());
  const auto &Iter = CheckOptions.find((NamePrefix + LocalName).str());
  if (Iter != CheckOptions.end())
    return StringRef(Iter->getValue().Value);
  return std::nullopt;
}

static ClangTidyOptions::OptionMap::const_iterator
findPriorityOption(const ClangTidyOptions::OptionMap &Options,
                   StringRef NamePrefix, StringRef LocalName,
                   llvm::StringSet<> *Collector) {
  if (Collector) {
    Collector->insert((NamePrefix + LocalName).str());
    Collector->insert(LocalName);
  }
  auto IterLocal = Options.find((NamePrefix + LocalName).str());
  auto IterGlobal = Options.find(LocalName);
  if (IterLocal == Options.end())
    return IterGlobal;
  if (IterGlobal == Options.end())
    return IterLocal;
  if (IterLocal->getValue().Priority >= IterGlobal->getValue().Priority)
    return IterLocal;
  return IterGlobal;
}

std::optional<StringRef>
ClangTidyCheck::OptionsView::getLocalOrGlobal(StringRef LocalName) const {
  auto Iter = findPriorityOption(CheckOptions, NamePrefix, LocalName,
                                 Context->getOptionsCollector());
  if (Iter != CheckOptions.end())
    return StringRef(Iter->getValue().Value);
  return std::nullopt;
}

static std::optional<bool> getAsBool(StringRef Value,
                                     const llvm::Twine &LookupName) {

  if (std::optional<bool> Parsed = llvm::yaml::parseBool(Value))
    return Parsed;
  // To maintain backwards compatability, we support parsing numbers as
  // booleans, even though its not supported in YAML.
  long long Number = 0;
  if (!Value.getAsInteger(10, Number))
    return Number != 0;
  return std::nullopt;
}

template <>
std::optional<bool>
ClangTidyCheck::OptionsView::get<bool>(StringRef LocalName) const {
  if (std::optional<StringRef> ValueOr = get(LocalName)) {
    if (auto Result = getAsBool(*ValueOr, NamePrefix + LocalName))
      return Result;
    diagnoseBadBooleanOption(NamePrefix + LocalName, *ValueOr);
  }
  return std::nullopt;
}

template <>
std::optional<bool>
ClangTidyCheck::OptionsView::getLocalOrGlobal<bool>(StringRef LocalName) const {
  auto Iter = findPriorityOption(CheckOptions, NamePrefix, LocalName,
                                 Context->getOptionsCollector());
  if (Iter != CheckOptions.end()) {
    if (auto Result = getAsBool(Iter->getValue().Value, Iter->getKey()))
      return Result;
    diagnoseBadBooleanOption(Iter->getKey(), Iter->getValue().Value);
  }
  return std::nullopt;
}

void ClangTidyCheck::OptionsView::store(ClangTidyOptions::OptionMap &Options,
                                        StringRef LocalName,
                                        StringRef Value) const {
  Options[(NamePrefix + LocalName).str()] = Value;
}

void ClangTidyCheck::OptionsView::storeInt(ClangTidyOptions::OptionMap &Options,
                                           StringRef LocalName,
                                           int64_t Value) const {
  store(Options, LocalName, llvm::itostr(Value));
}

void ClangTidyCheck::OptionsView::storeUnsigned(
    ClangTidyOptions::OptionMap &Options, StringRef LocalName,
    uint64_t Value) const {
  store(Options, LocalName, llvm::utostr(Value));
}

template <>
void ClangTidyCheck::OptionsView::store<bool>(
    ClangTidyOptions::OptionMap &Options, StringRef LocalName,
    bool Value) const {
  store(Options, LocalName, Value ? StringRef("true") : StringRef("false"));
}

std::optional<int64_t> ClangTidyCheck::OptionsView::getEnumInt(
    StringRef LocalName, ArrayRef<NameAndValue> Mapping, bool CheckGlobal,
    bool IgnoreCase) const {
  if (!CheckGlobal && Context->getOptionsCollector())
    Context->getOptionsCollector()->insert((NamePrefix + LocalName).str());
  auto Iter = CheckGlobal
                  ? findPriorityOption(CheckOptions, NamePrefix, LocalName,
                                       Context->getOptionsCollector())
                  : CheckOptions.find((NamePrefix + LocalName).str());
  if (Iter == CheckOptions.end())
    return std::nullopt;

  StringRef Value = Iter->getValue().Value;
  StringRef Closest;
  unsigned EditDistance = 3;
  for (const auto &NameAndEnum : Mapping) {
    if (IgnoreCase) {
      if (Value.equals_insensitive(NameAndEnum.second))
        return NameAndEnum.first;
    } else if (Value == NameAndEnum.second) {
      return NameAndEnum.first;
    } else if (Value.equals_insensitive(NameAndEnum.second)) {
      Closest = NameAndEnum.second;
      EditDistance = 0;
      continue;
    }
    unsigned Distance =
        Value.edit_distance(NameAndEnum.second, true, EditDistance);
    if (Distance < EditDistance) {
      EditDistance = Distance;
      Closest = NameAndEnum.second;
    }
  }
  if (EditDistance < 3)
    diagnoseBadEnumOption(Iter->getKey(), Iter->getValue().Value, Closest);
  else
    diagnoseBadEnumOption(Iter->getKey(), Iter->getValue().Value);
  return std::nullopt;
}

static constexpr llvm::StringLiteral ConfigWarning(
    "invalid configuration value '%0' for option '%1'%select{|; expected a "
    "bool|; expected an integer|; did you mean '%3'?}2");

void ClangTidyCheck::OptionsView::diagnoseBadBooleanOption(
    const Twine &Lookup, StringRef Unparsed) const {
  SmallString<64> Buffer;
  Context->configurationDiag(ConfigWarning)
      << Unparsed << Lookup.toStringRef(Buffer) << 1;
}

void ClangTidyCheck::OptionsView::diagnoseBadIntegerOption(
    const Twine &Lookup, StringRef Unparsed) const {
  SmallString<64> Buffer;
  Context->configurationDiag(ConfigWarning)
      << Unparsed << Lookup.toStringRef(Buffer) << 2;
}

void ClangTidyCheck::OptionsView::diagnoseBadEnumOption(
    const Twine &Lookup, StringRef Unparsed, StringRef Suggestion) const {
  SmallString<64> Buffer;
  auto Diag = Context->configurationDiag(ConfigWarning)
              << Unparsed << Lookup.toStringRef(Buffer);
  if (Suggestion.empty())
    Diag << 0;
  else
    Diag << 3 << Suggestion;
}

StringRef ClangTidyCheck::OptionsView::get(StringRef LocalName,
                                           StringRef Default) const {
  return get(LocalName).value_or(Default);
}

StringRef
ClangTidyCheck::OptionsView::getLocalOrGlobal(StringRef LocalName,
                                              StringRef Default) const {
  return getLocalOrGlobal(LocalName).value_or(Default);
}
} // namespace clang::tidy
