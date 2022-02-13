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
  bool visitEndIf(const PPEndIf *Directive) { return true; }

private:
  Derived &getDerived() { return *static_cast<Derived *>(this); }
};

template <class Derived>
bool PPTreeVisitor<Derived>::visitDirectives(const PPDirectiveList &List) {
  for (const PPDirective *Directive : List) {
    switch (Directive->getKind()) {
    case PPDirective::DK_Inclusion:
      if (!getDerived().visitInclusion(dyn_cast<PPInclusion>(Directive)))
        return false;
      break;
    case PPDirective::DK_Ident:
      if (!getDerived().visitIdent(dyn_cast<PPIdent>(Directive)))
        return false;
      break;
    case PPDirective::DK_Pragma:
      if (!getDerived().visitPragma(dyn_cast<PPPragma>(Directive)))
        return false;
      break;
    case PPDirective::DK_PragmaComment:
      if (!getDerived().visitPragmaComment(
              dyn_cast<PPPragmaComment>(Directive)))
        return false;
      break;
    case PPDirective::DK_PragmaDebug:
      if (!getDerived().visitPragmaDebug(dyn_cast<PPPragmaDebug>(Directive)))
        return false;
      break;
    case PPDirective::DK_PragmaDetectMismatch:
      if (!getDerived().visitPragmaDetectMismatch(
              dyn_cast<PPPragmaDetectMismatch>(Directive)))
        return false;
      break;
    case PPDirective::DK_PragmaMark:
      if (!getDerived().visitPragmaMark(dyn_cast<PPPragmaMark>(Directive)))
        return false;
      break;
    case PPDirective::DK_PragmaMessage:
      if (!getDerived().visitPragmaMessage(
              dyn_cast<PPPragmaMessage>(Directive)))
        return false;
      break;
    case PPDirective::DK_MacroDefined:
      if (!getDerived().visitMacroDefined(dyn_cast<PPMacroDefined>(Directive)))
        return false;
      break;
    case PPDirective::DK_MacroUndefined:
      if (!getDerived().visitMacroUndefined(
              dyn_cast<PPMacroUndefined>(Directive)))
        return false;
      break;
    case PPDirective::DK_If: {
      const PPIf *If = dyn_cast<PPIf>(Directive);
      if (!getDerived().visitIf(If))
        return false;
      if (!getDerived().visitDirectives(If->Directives))
        return false;
      break;
    }
    case PPDirective::DK_Else: {
      const PPElse *Else = dyn_cast<PPElse>(Directive);
      if (!getDerived().visitElse(Else))
        return false;
      if (!getDerived().visitDirectives(Else->Directives))
        return false;
      break;
    }
    case PPDirective::DK_ElseIf: {
      const PPElseIf *ElseIf = dyn_cast<PPElseIf>(Directive);
      if (!getDerived().visitElseIf(ElseIf))
        return false;
      break;
    }
    case PPDirective::DK_IfDef: {
      const PPIfDef *IfDef = dyn_cast<PPIfDef>(Directive);
      if (!getDerived().visitIfDef(IfDef))
        return false;
      if (!getDerived().visitDirectives(IfDef->Directives))
        return false;
      break;
    }
    case PPDirective::DK_IfNotDef: {
      const PPIfNotDef *IfNotDef = dyn_cast<PPIfNotDef>(Directive);
      if (!getDerived().visitIfNotDef(IfNotDef))
        return false;
      if (!getDerived().visitDirectives(IfNotDef->Directives))
        return false;
      break;
    }
    case PPDirective::DK_ElseIfDef: {
      const PPElseIfDef *ElseIfDef = dyn_cast<PPElseIfDef>(Directive);
      if (!getDerived().visitElseIfDef(ElseIfDef))
        return false;
      if (!getDerived().visitDirectives(ElseIfDef->Directives))
        return false;
      break;
    }
    case PPDirective::DK_ElseIfNotDef: {
      const PPElseIfNotDef *ElseIfNotDef = dyn_cast<PPElseIfNotDef>(Directive);
      if (!getDerived().visitElseIfNotDef(ElseIfNotDef))
        return false;
      if (!getDerived().visitDirectives(ElseIfNotDef->Directives))
        return false;
      break;
    }
    case PPDirective::DK_EndIf:
      if (!getDerived().visitEndIf(dyn_cast<PPEndIf>(Directive)))
        return false;
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
  std::string fieldIndent() const {
    return std::string((IndentLevel + 1) * 2, '.');
  }

  raw_ostream &Stream;
  const SourceManager &SM;
  size_t IndentLevel = 0;
};

} // namespace utils
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEVISITOR_H
