//===---------- PPTreeVisitor.h - clang-tidy ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEVISITOR_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEVISITOR_H

#include "PPTree.h"
#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace clang {

class SourceManager;

namespace tidy {
namespace utils {

template <class Derived> class PPTreeVisitor {
public:
  void visit(const PPTree *Tree) { visitDirectives(Tree->Directives); }
  bool visitDirectives(const PPDirectiveList &List);

  bool visitInclusion(const PPInclusion *Directive) { return true; }
  bool visitIdent(const PPIdent *Directive) { return true; }
  bool visitPragma(const PPPragma *Directive) { return true; }
  bool visitPragmaComment(const PPPragmaComment *Directive) { return true; }
  bool visitPragmaDebug(const PPPragmaDebug *Directive) { return true; }
  bool visitPragmaDetectMismatch(const PPPragmaDetectMismatch *Directive) {
    return true;
  }
  bool visitPragmaMark(const PPPragmaMark *Directive) { return true; }
  bool visitPragmaMessage(const PPPragmaMessage *Directive) { return true; }
  bool visitMacroDefined(const PPMacroDefined *Directive) { return true; }
  bool visitMacroUndefined(const PPMacroUndefined *Directive) { return true; }
  bool visitIf(const PPIf *Directive) { return true; }
  bool visitElse(const PPElse *Directive) { return true; }
  bool visitElseIf(const PPElseIf *Directive) { return true; }
  bool visitIfDef(const PPIfDef *Directive) { return true; }
  bool visitIfNotDef(const PPIfNotDef *Directive) { return true; }
  bool visitElseIfDef(const PPElseIfDef *Directive) { return true; }
  bool visitElseIfNotDef(const PPElseIfNotDef *Directive) { return true; }
  bool visitEndIf(const PPEndIf *Directive);

private:
  Derived &getDerived() { return *static_cast<Derived *>(this); }
};

template <class Derived>
bool PPTreeVisitor<Derived>::visitDirectives(const PPDirectiveList &List) {
  for (const PPDirective *Directive : List) {
    switch (Directive->getKind()) {
    case PPDirective::DK_Inclusion:
      getDerived().visitInclusion(dyn_cast<PPInclusion>(Directive));
      break;
    case PPDirective::DK_Ident:
      getDerived().visitIdent(dyn_cast<PPIdent>(Directive));
      break;
    case PPDirective::DK_Pragma:
      getDerived().visitPragma(dyn_cast<PPPragma>(Directive));
      break;
    case PPDirective::DK_PragmaComment:
      getDerived().visitPragmaComment(dyn_cast<PPPragmaComment>(Directive));
      break;
    case PPDirective::DK_PragmaDebug:
      getDerived().visitPragmaDebug(dyn_cast<PPPragmaDebug>(Directive));
      break;
    case PPDirective::DK_PragmaDetectMismatch:
      getDerived().visitPragmaDetectMismatch(
          dyn_cast<PPPragmaDetectMismatch>(Directive));
      break;
    case PPDirective::DK_PragmaMark:
      getDerived().visitPragmaMark(dyn_cast<PPPragmaMark>(Directive));
      break;
    case PPDirective::DK_PragmaMessage:
      getDerived().visitPragmaMessage(dyn_cast<PPPragmaMessage>(Directive));
      break;
    case PPDirective::DK_MacroDefined:
      getDerived().visitMacroDefined(dyn_cast<PPMacroDefined>(Directive));
      break;
    case PPDirective::DK_MacroUndefined:
      getDerived().visitMacroUndefined(dyn_cast<PPMacroUndefined>(Directive));
      break;
    case PPDirective::DK_If: {
      const PPIf *If = dyn_cast<PPIf>(Directive);
      getDerived().visitIf(If);
      getDerived().visitDirectives(If->Directives);
      break;
    }
    case PPDirective::DK_Else: {
      const PPElse *Else = dyn_cast<PPElse>(Directive);
      getDerived().visitElse(Else);
      getDerived().visitDirectives(Else->Directives);
      break;
    }
    case PPDirective::DK_ElseIf: {
      const PPElseIf *ElseIf = dyn_cast<PPElseIf>(Directive);
      getDerived().visitElseIf(ElseIf);
      break;
    }
    case PPDirective::DK_IfDef: {
      const PPIfDef *IfDef = dyn_cast<PPIfDef>(Directive);
      getDerived().visitIfDef(IfDef);
      getDerived().visitDirectives(IfDef->Directives);
      break;
    }
    case PPDirective::DK_IfNotDef: {
      const PPIfNotDef *IfNotDef = dyn_cast<PPIfNotDef>(Directive);
      getDerived().visitIfNotDef(IfNotDef);
      getDerived().visitDirectives(IfNotDef->Directives);
      break;
    }
    case PPDirective::DK_ElseIfDef: {
      const PPElseIfDef *ElseIfDef = dyn_cast<PPElseIfDef>(Directive);
      getDerived().visitElseIfDef(ElseIfDef);
      getDerived().visitDirectives(ElseIfDef->Directives);
      break;
    }
    case PPDirective::DK_ElseIfNotDef: {
      const PPElseIfNotDef *ElseIfNotDef = dyn_cast<PPElseIfNotDef>(Directive);
      getDerived().visitElseIfNotDef(ElseIfNotDef);
      getDerived().visitDirectives(ElseIfNotDef->Directives);
      break;
    }
    case PPDirective::DK_EndIf:
      getDerived().visitEndIf(dyn_cast<PPEndIf>(Directive));
      break;
    }
  }
  return true;
}

class PPTreePrinter : public PPTreeVisitor<PPTreePrinter> {
public:
  PPTreePrinter(raw_ostream &Stream, const SourceManager &SourceManager)
      : Stream(Stream), SM(SourceManager) {}

  bool visitDirectives(const PPDirectiveList &List);
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
  std::string indent() const { return std::string(IndentLevel * 2, '.'); }

  raw_ostream &Stream;
  const SourceManager &SM;
  size_t IndentLevel = 0;
};

} // namespace utils
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEVISITOR_H
