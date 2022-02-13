//===---------- PPTreeMatchers.h - clang-tidy -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEMATCHERS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEMATCHERS_H

#include "PPTree.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"

#include <string>

namespace clang {
namespace tidy {
namespace utils {

class PPMatchFinder;
using PPBoundNodes = ast_matchers::BoundNodes;
using PPBoundNodesMap = ast_matchers::internal::BoundNodesMap;

class PPBoundNodesTreeBuilder {
public:
  /// A visitor interface to visit all BoundNodes results for a
  /// BoundNodesTree.
  class Visitor {
  public:
    virtual ~Visitor() = default;

    /// Called multiple times during a single call to VisitMatches(...).
    ///
    /// 'BoundNodesView' contains the bound nodes for a single match.
    virtual void visitMatch(const PPBoundNodes &BoundNodesView) = 0;
  };

  /// Add a binding from an id to a node.
  void setBinding(StringRef Id, const DynTypedNode &DynNode) {
    if (Bindings.empty())
      Bindings.emplace_back();
    for (PPBoundNodesMap &Binding : Bindings)
      Binding.addNode(Id, DynNode);
  }

  /// Adds a branch in the tree.
  void addMatch(const PPBoundNodesTreeBuilder &Bindings);

  /// Visits all matches that this BoundNodesTree represents.
  ///
  /// The ownership of 'ResultVisitor' remains at the caller.
  void visitMatches(Visitor *ResultVisitor);

  template <typename ExcludePredicate>
  bool removeBindings(const ExcludePredicate &Predicate) {
    llvm::erase_if(Bindings, Predicate);
    return !Bindings.empty();
  }

  /// Imposes an order on BoundNodesTreeBuilders.
  bool operator<(const PPBoundNodesTreeBuilder &Other) const {
    return Bindings < Other.Bindings;
  }

  /// Returns \c true if this \c BoundNodesTreeBuilder can be compared,
  /// i.e. all stored node maps have memoization data.
  bool isComparable() const {
    return llvm::all_of(Bindings, std::mem_fn(&PPBoundNodesMap::isComparable));
  }

private:
  SmallVector<PPBoundNodesMap, 1> Bindings;
};

class PPDynMatcherInterface
    : public llvm::ThreadSafeRefCountedBase<PPDynMatcherInterface> {
public:
  virtual ~PPDynMatcherInterface() = default;

  /// Returns true if \p DynNode can be matched.
  ///
  /// May bind \p DynNode to an ID via \p Builder, or recurse into
  /// the AST via \p Finder.
  virtual bool dynMatches(const DynTypedNode &DynNode, PPMatchFinder *Finder,
                          PPBoundNodesTreeBuilder *Builder) const = 0;

  virtual llvm::Optional<clang::TraversalKind> traversalKind() const {
    return llvm::None;
  }
};

template <typename T> class PPMatcherInterface : public PPDynMatcherInterface {
public:
  /// Returns true if 'Node' can be matched.
  ///
  /// May bind 'Node' to an ID via 'Builder', or recurse into
  /// the AST via 'Finder'.
  virtual bool matches(const T &Node, PPMatchFinder *Finder,
                       PPBoundNodesTreeBuilder *Builder) const = 0;

  bool dynMatches(const DynTypedNode &DynNode, PPMatchFinder *Finder,
                  PPBoundNodesTreeBuilder *Builder) const override {
    return matches(DynNode.getUnchecked<T>(), Finder, Builder);
  }
};

template <typename> class PPMatcherT;

class PPDynTypedMatcher {
public:
  /// Takes ownership of the provided implementation pointer.
  template <typename T>
  PPDynTypedMatcher(PPMatcherInterface<T> *Implementation)
      : SupportedKind(ASTNodeKind::getFromNodeKind<T>()),
        RestrictKind(SupportedKind), Implementation(Implementation) {}

  /// Construct from a variadic function.
  enum VariadicOperator {
    /// Matches nodes for which all provided matchers match.
    VO_AllOf,

    /// Matches nodes for which at least one of the provided matchers
    /// matches.
    VO_AnyOf,

    /// Matches nodes for which at least one of the provided matchers
    /// matches, but doesn't stop at the first match.
    VO_EachOf,

    /// Matches any node but executes all inner matchers to find result
    /// bindings.
    VO_Optionally,

    /// Matches nodes that do not match the provided matcher.
    ///
    /// Uses the variadic matcher interface, but fails if
    /// InnerMatchers.size() != 1.
    VO_UnaryNot
  };

  static PPDynTypedMatcher
  constructVariadic(VariadicOperator Op, ASTNodeKind SupportedKind,
                    std::vector<PPDynTypedMatcher> InnerMatchers);

  static PPDynTypedMatcher
  constructRestrictedWrapper(const PPDynTypedMatcher &InnerMatcher,
                             ASTNodeKind RestrictKind);

  /// Get a "true" matcher for \p NodeKind.
  ///
  /// It only checks that the node is of the right kind.
  static PPDynTypedMatcher trueMatcher(ASTNodeKind NodeKind);

  void setAllowBind(bool AB) { AllowBind = AB; }

  /// Check whether this matcher could ever match a node of kind \p Kind.
  /// \return \c false if this matcher will never match such a node. Otherwise,
  /// return \c true.
  bool canMatchNodesOfKind(ASTNodeKind Kind) const;

  /// Return a matcher that points to the same implementation, but
  ///   restricts the node types for \p Kind.
  PPDynTypedMatcher dynCastTo(const ASTNodeKind Kind) const;

  /// Return a matcher that that points to the same implementation, but sets the
  ///   traversal kind.
  ///
  /// If the traversal kind is already set, then \c TK overrides it.
  PPDynTypedMatcher withTraversalKind(TraversalKind TK);

  /// Returns true if the matcher matches the given \c DynNode.
  bool matches(const DynTypedNode &DynNode, PPMatchFinder *Finder,
               PPBoundNodesTreeBuilder *Builder) const;

  /// Same as matches(), but skips the kind check.
  ///
  /// It is faster, but the caller must ensure the node is valid for the
  /// kind of this matcher.
  bool matchesNoKindCheck(const DynTypedNode &DynNode, PPMatchFinder *Finder,
                          PPBoundNodesTreeBuilder *Builder) const;

  /// Bind the specified \p ID to the matcher.
  /// \return A new matcher with the \p ID bound to it if this matcher supports
  ///   binding. Otherwise, returns an empty \c Optional<>.
  llvm::Optional<PPDynTypedMatcher> tryBind(StringRef ID) const;

  /// Returns a unique \p ID for the matcher.
  ///
  /// Casting a Matcher<T> to Matcher<U> creates a matcher that has the
  /// same \c Implementation pointer, but different \c RestrictKind. We need to
  /// include both in the ID to make it unique.
  ///
  /// \c MatcherIDType supports operator< and provides strict weak ordering.
  using MatcherIDType = std::pair<ASTNodeKind, uint64_t>;
  MatcherIDType getID() const {
    /// FIXME: Document the requirements this imposes on matcher
    /// implementations (no new() implementation_ during a Matches()).
    return std::make_pair(RestrictKind,
                          reinterpret_cast<uint64_t>(Implementation.get()));
  }

  /// Returns the type this matcher works on.
  ///
  /// \c matches() will always return false unless the node passed is of this
  /// or a derived type.
  ASTNodeKind getSupportedKind() const { return SupportedKind; }

  /// Returns \c true if the passed \c DynTypedMatcher can be converted
  ///   to a \c Matcher<T>.
  ///
  /// This method verifies that the underlying matcher in \c Other can process
  /// nodes of types T.
  template <typename T> bool canConvertTo() const {
    return canConvertTo(ASTNodeKind::getFromNodeKind<T>());
  }
  bool canConvertTo(ASTNodeKind To) const;

  /// Construct a \c Matcher<T> interface around the dynamic matcher.
  ///
  /// This method asserts that \c canConvertTo() is \c true. Callers
  /// should call \c canConvertTo() first to make sure that \c this is
  /// compatible with T.
  template <typename T> PPMatcherT<T> convertTo() const {
    assert(canConvertTo<T>());
    return unconditionalConvertTo<T>();
  }

  /// Same as \c convertTo(), but does not check that the underlying
  ///   matcher can handle a value of T.
  ///
  /// If it is not compatible, then this matcher will never match anything.
  template <typename T> PPMatcherT<T> unconditionalConvertTo() const;

  /// Returns the \c TraversalKind respected by calls to `match()`, if any.
  ///
  /// Most matchers will not have a traversal kind set, instead relying on the
  /// surrounding context. For those, \c llvm::None is returned.
  llvm::Optional<clang::TraversalKind> getTraversalKind() const {
    return Implementation->traversalKind();
  }

private:
  PPDynTypedMatcher(ASTNodeKind SupportedKind, ASTNodeKind RestrictKind,
                    IntrusiveRefCntPtr<PPDynMatcherInterface> Implementation)
      : SupportedKind(SupportedKind), RestrictKind(RestrictKind),
        Implementation(std::move(Implementation)) {}

  bool AllowBind = false;
  ASTNodeKind SupportedKind;

  /// A potentially stricter node kind.
  ///
  /// It allows to perform implicit and dynamic cast of matchers without
  /// needing to change \c Implementation.
  ASTNodeKind RestrictKind;
  IntrusiveRefCntPtr<PPDynMatcherInterface> Implementation;
};

template <typename T> class PPMatcherT {
public:
  /// Takes ownership of the provided implementation pointer.
  explicit PPMatcherT(PPMatcherInterface<T> *Implementation)
      : Implementation(Implementation) {}

  /// Implicitly converts \c Other to a PPMatcherT<T>.
  ///
  /// Requires \c T to be derived from \c From.
  template <typename From>
  PPMatcherT(const PPMatcherT<From> &Other,
             std::enable_if_t<std::is_base_of<From, T>::value &&
                              !std::is_same<From, T>::value> * = nullptr)
      : Implementation(restrictMatcher(Other.Implementation)) {
#if 0
    assert(Implementation.getSupportedKind().isSame(
        PPNodeKind::getFromNodeKind<T>()));
#endif
  }

  /// Convert \c this into a \c PPMatcherT<T> by applying dyn_cast<> to the
  /// argument.
  /// \c To must be a base class of \c T.
  template <typename To> PPMatcherT<To> dynCastTo() const & {
    static_assert(std::is_base_of<To, T>::value, "Invalid dynCast call.");
    return PPMatcherT<To>(Implementation);
  }

  template <typename To> PPMatcherT<To> dynCastTo() && {
    static_assert(std::is_base_of<To, T>::value, "Invalid dynCast call.");
    return PPMatcherT<To>(std::move(Implementation));
  }

  /// Forwards the call to the underlying MatcherInterface<T> pointer.
  bool matches(const T &Node, PPMatchFinder *Finder,
               PPBoundNodesTreeBuilder *Builder) const {
    return Implementation.matches(DynTypedNode::create(Node), Finder, Builder);
  }

  /// Returns an ID that uniquely identifies the matcher.
  PPDynTypedMatcher::MatcherIDType getID() const {
    return Implementation.getID();
  }

  /// Extract the dynamic matcher.
  ///
  /// The returned matcher keeps the same restrictions as \c this and remembers
  /// that it is meant to support nodes of type \c T.
  operator PPDynTypedMatcher() const & { return Implementation; }

  operator PPDynTypedMatcher() && { return std::move(Implementation); }

private:
  // For Matcher<T> <=> Matcher<U> conversions.
  template <typename U> friend class PPMatcherT;

  // For DynTypedMatcher::unconditionalConvertTo<T>.
  friend class PPDynTypedMatcher;

  static PPDynTypedMatcher restrictMatcher(const PPDynTypedMatcher &Other) {
    return Other.dynCastTo(ASTNodeKind::getFromNodeKind<T>());
  }

  explicit PPMatcherT(const PPDynTypedMatcher &Implementation)
      : Implementation(restrictMatcher(Implementation)) {
    assert(this->Implementation.getSupportedKind().isSame(
        ASTNodeKind::getFromNodeKind<T>()));
  }

  PPDynTypedMatcher Implementation;
}; // class PPMatcherT

template <typename T>
using PPMatcher =
    ast_matchers::internal::VariadicDynCastAllOfMatcher<PPDirective, T>;

extern const PPMatcher<PPInclusion> ppInclusion;
extern const PPMatcher<PPIdent> identDirective;
extern const PPMatcher<PPPragma> pragmaDirective;
extern const PPMatcher<PPPragmaComment> pragmaCommentDirective;
extern const PPMatcher<PPPragmaDebug> pragmaDebugDirective;
extern const PPMatcher<PPPragmaDetectMismatch> pragmaDetectMismatchDirective;
extern const PPMatcher<PPPragmaMark> pragmaMarkDirective;
extern const PPMatcher<PPPragmaMessage> pragmaMessage;
extern const PPMatcher<PPMacroDefined> macroDefinedDirective;
extern const PPMatcher<PPMacroUndefined> macroUndefinedDirective;
extern const PPMatcher<PPIf> ifDirective;
extern const PPMatcher<PPElse> elseDirective;
extern const PPMatcher<PPElseIf> elseIfDirective;
extern const PPMatcher<PPIfDef> ifDefDirective;
extern const PPMatcher<PPIfNotDef> ifNotDefDirective;
extern const PPMatcher<PPElseIfDef> elseIfDefDirective;
extern const PPMatcher<PPElseIfNotDef> elseIfNotDefDirective;
extern const PPMatcher<PPEndIf> endIfDirective;

using PPDirectiveMatcher = ast_matchers::internal::Matcher<PPDirective>;

class DirectiveMatchFinder {
public:
  struct MatchResult {
    MatchResult(const ast_matchers::BoundNodes &Nodes,
                SourceManager &SourceManager)
        : Nodes(Nodes), SourceManager(SourceManager) {}

    const ast_matchers::BoundNodes Nodes;
    SourceManager &SourceManager;
  };

  void addMatcher(const PPDirectiveMatcher &NodeMatch) {
    Matchers.emplace_back(&NodeMatch);
  }

  void match(const PPTree *Tree);

private:
  std::vector<const PPDirectiveMatcher *> Matchers;
};

} // namespace utils
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREEMATCHERS_H
