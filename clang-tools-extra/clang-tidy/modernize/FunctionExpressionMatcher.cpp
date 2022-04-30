//===--- FunctionExpressionMatcher.cpp - clang-tidy -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FunctionExpressionMatcher.h"

#include "clang/Basic/IdentifierTable.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Token.h"

#include <cctype>
#include <stdexcept>

namespace clang {
namespace tidy {
namespace modernize {

// Validate that this literal token is a valid integer literal.  A literal token
// could be a floating-point token, which isn't acceptable as a value for an
// enumeration.  A floating-point token must either have a decimal point or an
// exponent ('E' or 'P').
static bool isIntegralConstant(const Token &Token) {
  const char *Begin = Token.getLiteralData();
  const char *End = Begin + Token.getLength();

  // Not a hexadecimal floating-point literal.
  if (Token.getLength() > 2 && Begin[0] == '0' && std::toupper(Begin[1]) == 'X')
    return std::none_of(Begin + 2, End, [](char C) {
      return C == '.' || std::toupper(C) == 'P';
    });

  // Not a decimal floating-point literal or complex literal.
  return std::none_of(Begin, End, [](char C) {
    return C == '.' || std::toupper(C) == 'E' || std::toupper(C) == 'I';
  });
}

bool FunctionExpressionMatcher::advance() {
  ++Current;
  return Current != End;
}

bool FunctionExpressionMatcher::consume(tok::TokenKind Kind) {
  if (Current->is(Kind)) {
    ++Current;
    return true;
  }

  return false;
}

bool FunctionExpressionMatcher::nonTerminalChainedExpr(
    bool (FunctionExpressionMatcher::*NonTerminal)(),
    const std::function<bool(Token)> &IsKind) {
  if (!(this->*NonTerminal)())
    return false;
  if (Current == End)
    return true;

  while (Current != End) {
    if (!IsKind(*Current))
      break;

    if (!advance())
      return false;

    if (!(this->*NonTerminal)())
      return false;
  }

  return true;
}

// Advance over unary operators.
bool FunctionExpressionMatcher::unaryOperator() {
  if (Current->isOneOf(tok::TokenKind::minus, tok::TokenKind::plus,
                       tok::TokenKind::tilde, tok::TokenKind::exclaim)) {
    return advance();
  }

  return true;
}

static StringRef getTokenName(const Token &Tok) {
  return Tok.is(tok::raw_identifier) ? Tok.getRawIdentifier()
                                     : Tok.getIdentifierInfo()->getName();
}

bool FunctionExpressionMatcher::unaryExpr() {
  if (!unaryOperator())
    return false;

  if (consume(tok::TokenKind::l_paren)) {
    if (Current == End)
      return false;

    if (!expr())
      return false;

    if (Current == End)
      return false;

    return consume(tok::TokenKind::r_paren);
  }

  if (!Current->isLiteral()) {
    if (Current->isAnyIdentifier()) {
      StringRef Name = getTokenName(*Current);
      if (any_of(AllowedIdentifiers, [Name](const IdentifierInfo *Id) {
            return Name == Id->getName();
          })) {
        ++Current;
        return true;
      }
    }

    return false;
  }

  ++Current;
  return true;
}

bool FunctionExpressionMatcher::multiplicativeExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::star, tok::TokenKind::slash,
                                tok::TokenKind::percent>(
      &FunctionExpressionMatcher::unaryExpr);
}

bool FunctionExpressionMatcher::additiveExpr() {
  return nonTerminalChainedExpr<tok::plus, tok::minus>(
      &FunctionExpressionMatcher::multiplicativeExpr);
}

bool FunctionExpressionMatcher::shiftExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::lessless,
                                tok::TokenKind::greatergreater>(
      &FunctionExpressionMatcher::additiveExpr);
}

bool FunctionExpressionMatcher::compareExpr() {
  if (!shiftExpr())
    return false;
  if (Current == End)
    return true;

  if (Current->is(tok::TokenKind::spaceship)) {
    if (!advance())
      return false;

    if (!shiftExpr())
      return false;
  }

  return true;
}

bool FunctionExpressionMatcher::relationalExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::less, tok::TokenKind::greater,
                                tok::TokenKind::lessequal,
                                tok::TokenKind::greaterequal>(
      &FunctionExpressionMatcher::compareExpr);
}

bool FunctionExpressionMatcher::equalityExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::equalequal,
                                tok::TokenKind::exclaimequal>(
      &FunctionExpressionMatcher::relationalExpr);
}

bool FunctionExpressionMatcher::andExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::amp>(
      &FunctionExpressionMatcher::equalityExpr);
}

bool FunctionExpressionMatcher::exclusiveOrExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::caret>(
      &FunctionExpressionMatcher::andExpr);
}

bool FunctionExpressionMatcher::inclusiveOrExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::pipe>(
      &FunctionExpressionMatcher::exclusiveOrExpr);
}

bool FunctionExpressionMatcher::logicalAndExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::ampamp>(
      &FunctionExpressionMatcher::inclusiveOrExpr);
}

bool FunctionExpressionMatcher::logicalOrExpr() {
  return nonTerminalChainedExpr<tok::TokenKind::pipepipe>(
      &FunctionExpressionMatcher::logicalAndExpr);
}

bool FunctionExpressionMatcher::conditionalExpr() {
  if (!logicalOrExpr())
    return false;
  if (Current == End)
    return true;

  if (Current->is(tok::TokenKind::question)) {
    if (!advance())
      return false;

    // A gcc extension allows x ? : y as a synonym for x ? x : y.
    if (Current->is(tok::TokenKind::colon)) {
      if (!advance())
        return false;

      if (!expr())
        return false;

      return true;
    }

    if (!expr())
      return false;
    if (Current == End)
      return false;

    if (!Current->is(tok::TokenKind::colon))
      return false;

    if (!advance())
      return false;

    if (!expr())
      return false;
  }
  return true;
}

bool FunctionExpressionMatcher::expr() { return conditionalExpr(); }

FunctionExpressionMatcher::FunctionExpressionMatcher(
    ArrayRef<Token> Definition, ArrayRef<const IdentifierInfo *> AllowedIdentifiers)
    : Current(Definition.begin()), End(Definition.end()),
      AllowedIdentifiers(AllowedIdentifiers) {}

bool FunctionExpressionMatcher::match() {
  return expr() && Current == End;
}

} // namespace modernize
} // namespace tidy
} // namespace clang
