//===--- FunctionExpressionMatcher.h - clang-tidy -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_FUNCTION_EXPRESSION_MATCHER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MODERNIZE_FUNCTION_EXPRESSION_MATCHER_H

#include <clang/Lex/Token.h>
#include <llvm/ADT/ArrayRef.h>

namespace clang {

class IdentifierInfo;

namespace tidy {
namespace modernize {

// Parses an array of tokens and returns true if they conform to the rules of
// C++ for whole expressions involving integral literals.  Follows the operator
// precedence rules of C++.
class FunctionExpressionMatcher {
public:
  FunctionExpressionMatcher(ArrayRef<Token> Definition, ArrayRef<const IdentifierInfo *> AllowedIdentifiers);

  bool match();

private:
  bool advance();
  bool consume(tok::TokenKind Kind);
  bool nonTerminalChainedExpr(
      bool (FunctionExpressionMatcher::*NonTerminal)(),
      const std::function<bool(Token)> &IsKind);
  template <tok::TokenKind Kind>
  bool nonTerminalChainedExpr(
      bool (FunctionExpressionMatcher::*NonTerminal)()) {
    return nonTerminalChainedExpr(NonTerminal,
                                  [](Token Tok) { return Tok.is(Kind); });
  }
  template <tok::TokenKind K1, tok::TokenKind K2, tok::TokenKind... Ks>
  bool nonTerminalChainedExpr(
      bool (FunctionExpressionMatcher::*NonTerminal)()) {
    return nonTerminalChainedExpr(
        NonTerminal, [](Token Tok) { return Tok.isOneOf(K1, K2, Ks...); });
  }

  bool unaryOperator();
  bool unaryExpr();
  bool multiplicativeExpr();
  bool additiveExpr();
  bool shiftExpr();
  bool compareExpr();
  bool relationalExpr();
  bool equalityExpr();
  bool andExpr();
  bool exclusiveOrExpr();
  bool inclusiveOrExpr();
  bool logicalAndExpr();
  bool logicalOrExpr();
  bool conditionalExpr();
  bool expr();

  ArrayRef<Token>::iterator Current;
  ArrayRef<Token>::iterator End;
  ArrayRef<const IdentifierInfo *> AllowedIdentifiers;
};

} // namespace modernize
} // namespace tidy
} // namespace clang

#endif
