//===-------- PPTreeVisitor.cpp - clang-tidy ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PPTreeVisitor.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {
namespace tidy {
namespace utils {

static std::string printToString(const SourceManager &SM, CharSourceRange R) {
  return R.getBegin().printToString(SM) + ", " + R.getEnd().printToString(SM);
}

bool PPTreePrinter::visitDirectives(const PPDirectiveList &List) {
  ++IndentLevel;
  PPTreeVisitor<PPTreePrinter>::visitDirectives(List);
  --IndentLevel;
  return true;
}

bool PPTreePrinter::visitInclusion(const PPInclusion *D) {
  Stream << "Inclusion\n"
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
  return true;
}

bool PPTreePrinter::visitIdent(const PPIdent *D) {
  Stream << "Ident\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Str << '\n';
  return true;
}
bool PPTreePrinter::visitPragma(const PPPragma *D) {
  Stream << "Pragma\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << "Introducer " << D->Introducer << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaComment(const PPPragmaComment *D) {
  Stream << "Comment\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Kind->getName().str() << '\n'
         << indent() << D->Str << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaMark(const PPPragmaMark *D) {
  Stream << "Mark\n"
         << D->Loc.printToString(SM) << '\n'
         << indent() << D->Trivia << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaDetectMismatch(const PPPragmaDetectMismatch *D) {
  Stream << "Detect Mismatch\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name << '\n'
         << indent() << D->Value << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaDebug(const PPPragmaDebug *D) {
  Stream << "Debug\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->DebugType << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaMessage(const PPPragmaMessage *D) {
  Stream << "Message\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Namespace << '\n'
         << indent() << D->Kind << '\n'
         << indent() << D->Str << '\n';
  return true;
}
bool PPTreePrinter::visitMacroDefined(const PPMacroDefined *D) {
  Stream << "Macro Defined\n"
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  return true;
}
bool PPTreePrinter::visitMacroUndefined(const PPMacroUndefined *D) {
  Stream << "Macro Undefined\n"
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  return true;
}
bool PPTreePrinter::visitIf(const PPIf *D) {
  Stream << "If\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->ConditionRange.getBegin().printToString(SM) << ", "
         << D->ConditionRange.getEnd().printToString(SM) << '\n'
         << indent() << D->ConditionValue << '\n';
  return true;
}
bool PPTreePrinter::visitElse(const PPElse *D) {
  Stream << "Else\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->IfLoc.printToString(SM) << '\n';
  return true;
}
bool PPTreePrinter::visitElseIf(const PPElseIf *D) {
  Stream << "ElseIf\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->ConditionRange.getBegin().printToString(SM) << ", "
         << D->ConditionRange.getEnd().printToString(SM) << '\n'
         << indent() << D->ConditionValue << '\n'
         << indent() << D->IfLoc.printToString(SM) << '\n';
  return true;
}
bool PPTreePrinter::visitIfDef(const PPIfDef *D) {
  Stream << "IfDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  return true;
}
bool PPTreePrinter::visitIfNotDef(const PPIfNotDef *D) {
  Stream << "IfNotDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  return true;
}
bool PPTreePrinter::visitElseIfDef(const PPElseIfDef *D) {
  Stream << "ElseIfDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  return true;
}
bool PPTreePrinter::visitElseIfNotDef(const PPElseIfNotDef *D) {
  Stream << "ElseIfNotDef\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->Name.getIdentifierInfo()->getName().str() << '\n';
  return true;
}
bool PPTreePrinter::visitEndIf(const PPEndIf *D) {
  Stream << "EndIf\n"
         << indent() << D->Loc.printToString(SM) << '\n'
         << indent() << D->IfLoc.printToString(SM) << '\n';
  return true;
}

} // namespace utils
} // namespace tidy
} // namespace clang
