//===-------- PPTree.cpp - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PPTreeMatchers.h"
#include "PPTreeVisitor.h"

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

class MatchPPTreeVisitor : public PPTreeVisitor<MatchPPTreeVisitor> {
};

void DirectiveMatchFinder::match(const PPTree *Tree) {
  MatchPPTreeVisitor Visitor;
  Visitor.visit(Tree);
}

} // namespace utils
} // namespace tidy
} // namespace clang
