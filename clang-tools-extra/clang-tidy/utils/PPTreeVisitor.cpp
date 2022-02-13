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
  Stream << indent() << "Inclusion\n"
         << fieldIndent() << D->HashLoc.printToString(SM) << '\n'
         << fieldIndent() << D->IncludeTok.getIdentifierInfo()->getName().str()
         << '\n'
         << fieldIndent() << D->FileName << '\n'
         << fieldIndent() << (D->IsAngled ? "Angled\n" : "") << fieldIndent()
         << printToString(SM, D->FilenameRange) << '\n'
         << fieldIndent() << D->File->getDir()->getName().str() << '\n'
         << fieldIndent() << D->SearchPath << '\n'
         << fieldIndent() << D->RelativePath << '\n'
         << fieldIndent() << (D->Imported != nullptr ? "<Imported>\n" : "")
         << fieldIndent() << "FileType " << fieldIndent() << D->FileType
         << '\n';
  return true;
}

bool PPTreePrinter::visitIdent(const PPIdent *D) {
  Stream << indent() << "Ident\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Str << '\n';
  return true;
}
bool PPTreePrinter::visitPragma(const PPPragma *D) {
  Stream << indent() << "Pragma\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << "Introducer " << D->Introducer << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaComment(const PPPragmaComment *D) {
  Stream << indent() << "Comment\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Kind->getName().str() << '\n'
         << fieldIndent() << D->Str << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaMark(const PPPragmaMark *D) {
  Stream << indent() << "Mark\n"
         << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Trivia << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaDetectMismatch(const PPPragmaDetectMismatch *D) {
  Stream << indent() << "Detect Mismatch\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Name << '\n'
         << fieldIndent() << D->Value << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaDebug(const PPPragmaDebug *D) {
  Stream << indent() << "Debug\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->DebugType << '\n';
  return true;
}
bool PPTreePrinter::visitPragmaMessage(const PPPragmaMessage *D) {
  Stream << indent() << "Message\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Namespace << '\n'
         << fieldIndent() << D->Kind << '\n'
         << fieldIndent() << D->Str << '\n';
  return true;
}
bool PPTreePrinter::visitMacroDefined(const PPMacroDefined *D) {
  Stream << indent() << "Macro Defined\n"
         << fieldIndent() << D->Name.getIdentifierInfo()->getName().str()
         << '\n';
  return true;
}
bool PPTreePrinter::visitMacroUndefined(const PPMacroUndefined *D) {
  Stream << indent() << "Macro Undefined\n"
         << fieldIndent() << D->Name.getIdentifierInfo()->getName().str()
         << '\n';
  return true;
}
bool PPTreePrinter::visitIf(const PPIf *D) {
  Stream << indent() << "If\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->ConditionRange.getBegin().printToString(SM)
         << ", " << D->ConditionRange.getEnd().printToString(SM) << '\n'
         << fieldIndent() << D->ConditionValue << '\n';
  return true;
}
bool PPTreePrinter::visitElse(const PPElse *D) {
  Stream << indent() << "Else\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->IfLoc.printToString(SM) << '\n';
  return true;
}
bool PPTreePrinter::visitElseIf(const PPElseIf *D) {
  Stream << indent() << "ElseIf\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->ConditionRange.getBegin().printToString(SM)
         << ", " << D->ConditionRange.getEnd().printToString(SM) << '\n'
         << fieldIndent() << D->ConditionValue << '\n'
         << fieldIndent() << D->IfLoc.printToString(SM) << '\n';
  return true;
}
bool PPTreePrinter::visitIfDef(const PPIfDef *D) {
  Stream << indent() << "IfDef\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Name.getIdentifierInfo()->getName().str()
         << '\n';
  return true;
}
bool PPTreePrinter::visitIfNotDef(const PPIfNotDef *D) {
  Stream << indent() << "IfNotDef\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Name.getIdentifierInfo()->getName().str()
         << '\n';
  return true;
}
bool PPTreePrinter::visitElseIfDef(const PPElseIfDef *D) {
  Stream << indent() << "ElseIfDef\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Name.getIdentifierInfo()->getName().str()
         << '\n';
  return true;
}
bool PPTreePrinter::visitElseIfNotDef(const PPElseIfNotDef *D) {
  Stream << indent() << "ElseIfNotDef\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->Name.getIdentifierInfo()->getName().str()
         << '\n';
  return true;
}
bool PPTreePrinter::visitEndIf(const PPEndIf *D) {
  Stream << indent() << "EndIf\n"
         << fieldIndent() << D->Loc.printToString(SM) << '\n'
         << fieldIndent() << D->IfLoc.printToString(SM) << '\n';
  return true;
}

} // namespace utils
} // namespace tidy
} // namespace clang
