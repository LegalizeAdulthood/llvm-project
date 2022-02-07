//===---------- PPTree.h - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"
#include <string>

namespace clang {
namespace tidy {
namespace utils {

class PPDirective {
public:
  enum DirectiveKind {
    DK_Inclusion,
    DK_Ident,
    DK_Pragma,
    DK_PragmaComment,
    DK_PragmaDebug,
    DK_PragmaDetectMismatch,
    DK_PragmaMark,
    DK_PragmaMessage,
    DK_MacroDefined,
    DK_MacroUndefined,
    DK_If,
    DK_Else,
    DK_ElseIf,
    DK_IfDef,
    DK_IfNotDef,
    DK_ElseIfDef,
    DK_ElseIfNotDef,
    DK_EndIf,
  };

  virtual ~PPDirective() = default;

  DirectiveKind getKind() const { return Kind; }

protected:
  PPDirective(DirectiveKind Kind) : Kind(Kind) {}

private:
  DirectiveKind Kind;
};

using PPDirectiveStorage = std::vector<PPDirective *>;

class PPDirectiveList {
public:
  void add(PPDirective *Dir) { Directives.emplace_back(Dir); }

  ~PPDirectiveList() {
    for (PPDirective *Dir : Directives)
      delete Dir;
    Directives.clear();
  }

  PPDirectiveStorage::iterator begin() { return Directives.begin(); }
  PPDirectiveStorage::iterator end() { return Directives.end(); }

  PPDirectiveStorage::const_iterator begin() const {
    return Directives.begin();
  }
  PPDirectiveStorage::const_iterator end() const { return Directives.end(); }

  size_t size() const { return Directives.size(); }

private:
  PPDirectiveStorage Directives;
};

class PPInclusion : public PPDirective {
public:
  PPInclusion(SourceLocation HashLoc, Token IncludeTok, StringRef FileName,
              bool IsAngled, CharSourceRange FilenameRange,
              const FileEntry *File, StringRef SearchPath,
              StringRef RelativePath, const Module *Imported,
              SrcMgr::CharacteristicKind FileType)
      : PPDirective(DK_Inclusion), HashLoc(HashLoc), IncludeTok(IncludeTok),
        FileName(FileName.str()), IsAngled(IsAngled),
        FilenameRange(FilenameRange), File(File), SearchPath(SearchPath.str()),
        RelativePath(RelativePath.str()), Imported(Imported),
        FileType(FileType) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_Inclusion;
  }

  SourceLocation HashLoc;
  Token IncludeTok;
  std::string FileName;
  bool IsAngled;
  CharSourceRange FilenameRange;
  const FileEntry *File;
  std::string SearchPath;
  std::string RelativePath;
  const Module *Imported;
  SrcMgr::CharacteristicKind FileType;
};

class PPIdent : public PPDirective {
public:
  PPIdent(SourceLocation Loc, StringRef Str)
      : PPDirective(DK_Ident), Loc(Loc), Str(Str.str()) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_Ident; }

  SourceLocation Loc;
  std::string Str;
};

class PPPragma : public PPDirective {
public:
  PPPragma(SourceLocation Loc, PragmaIntroducerKind Introducer)
      : PPDirective(DK_Pragma), Loc(Loc), Introducer(Introducer) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_Pragma;
  }

  SourceLocation Loc;
  PragmaIntroducerKind Introducer;
};

class PPPragmaComment : public PPDirective {
public:
  PPPragmaComment(SourceLocation Loc, const IdentifierInfo *Kind, StringRef Str)
      : PPDirective(DK_PragmaComment), Loc(Loc), Kind(Kind), Str(Str.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaComment;
  }

  SourceLocation Loc;
  const IdentifierInfo *Kind;
  std::string Str;
};

class PPPragmaDebug : public PPDirective {
public:
  PPPragmaDebug(SourceLocation Loc, StringRef DebugType)
      : PPDirective(DK_PragmaDebug), Loc(Loc), DebugType(DebugType.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaDebug;
  }

  SourceLocation Loc;
  std::string DebugType;
};

class PPPragmaDetectMismatch : public PPDirective {
public:
  PPPragmaDetectMismatch(SourceLocation Loc, StringRef Name, StringRef Value)
      : PPDirective(DK_PragmaDetectMismatch), Loc(Loc), Name(Name.str()),
        Value(Value.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaDetectMismatch;
  }

  SourceLocation Loc;
  std::string Name;
  std::string Value;
};

class PPPragmaMark : public PPDirective {
public:
  PPPragmaMark(SourceLocation Loc, StringRef Trivia)
      : PPDirective(DK_PragmaMark), Loc(Loc), Trivia(Trivia.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaMark;
  }

  SourceLocation Loc;
  std::string Trivia;
};

class PPPragmaMessage : public PPDirective {
public:
  PPPragmaMessage(SourceLocation Loc, StringRef Namespace,
                  PPCallbacks::PragmaMessageKind Kind, StringRef Str)
      : PPDirective(DK_PragmaMessage), Loc(Loc), Namespace(Namespace.str()),
        Kind(Kind), Str(Str.str()) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_PragmaMessage;
  }

  SourceLocation Loc;
  std::string Namespace;
  PPCallbacks::PragmaMessageKind Kind;
  std::string Str;
};

class PPMacroDefined : public PPDirective {
public:
  PPMacroDefined(const Token &MacroNameTok, const MacroDirective *MD)
      : PPDirective(DK_MacroDefined), Name(MacroNameTok), MD(MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_MacroDefined;
  }

  Token Name;
  const MacroDirective *MD;
};

class PPMacroUndefined : public PPDirective {
public:
  PPMacroUndefined(Token Name, const MacroDefinition *MD,
                   const MacroDirective *Undef)
      : PPDirective(DK_MacroUndefined), Name(Name), MD(MD), Undef(Undef) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_MacroUndefined;
  }

  Token Name;
  const MacroDefinition *MD;
  const MacroDirective *Undef;
};

class PPIf : public PPDirective {
public:
  PPIf(SourceLocation Loc, SourceRange ConditionRange,
       PPCallbacks::ConditionValueKind ConditionValue)
      : PPDirective(DK_If), Loc(Loc), ConditionRange(ConditionRange),
        ConditionValue(ConditionValue) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_If; }

  SourceLocation Loc;
  SourceRange ConditionRange;
  PPCallbacks::ConditionValueKind ConditionValue;
  PPDirectiveList Directives;
};

class PPElse : public PPDirective {
public:
  PPElse(SourceLocation Loc, SourceLocation IfLoc)
      : PPDirective(DK_Else), Loc(Loc), IfLoc(IfLoc) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_Else; }

  SourceLocation Loc;
  SourceLocation IfLoc;
  PPDirectiveList Directives;
};

class PPElseIf : public PPDirective {
public:
  PPElseIf(SourceLocation Loc, SourceRange ConditionRange,
           PPCallbacks::ConditionValueKind ConditionValue, SourceLocation IfLoc)
      : PPDirective(DK_ElseIf), Loc(Loc), ConditionRange(ConditionRange),
        ConditionValue(ConditionValue), IfLoc(IfLoc) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_ElseIf;
  }

  SourceLocation Loc;
  SourceRange ConditionRange;
  PPCallbacks::ConditionValueKind ConditionValue;
  SourceLocation IfLoc;
  PPDirectiveList Directives;
};

class PPIfDef : public PPDirective {
public:
  PPIfDef(SourceLocation Loc, const Token &MacroNameTok,
          const MacroDefinition &MD)
      : PPDirective(DK_IfDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_IfDef; }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPIfNotDef : public PPDirective {
public:
  PPIfNotDef(SourceLocation Loc, const Token &MacroNameTok,
             const MacroDefinition &MD)
      : PPDirective(DK_IfNotDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_IfNotDef;
  }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPElseIfDef : public PPDirective {
public:
  PPElseIfDef(SourceLocation Loc, const Token &MacroNameTok,
              const MacroDefinition &MD)
      : PPDirective(DK_ElseIfDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_ElseIfDef;
  }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPElseIfNotDef : public PPDirective {
public:
  PPElseIfNotDef(SourceLocation Loc, const Token &MacroNameTok,
                 const MacroDefinition &MD)
      : PPDirective(DK_ElseIfNotDef), Loc(Loc), Name(MacroNameTok), MD(&MD) {}

  static bool classof(const PPDirective *D) {
    return D->getKind() == DK_ElseIfNotDef;
  }

  SourceLocation Loc;
  Token Name;
  const MacroDefinition *MD;
  PPDirectiveList Directives;
};

class PPEndIf : public PPDirective {
public:
  PPEndIf(SourceLocation Loc, SourceLocation IfLoc)
      : PPDirective(DK_EndIf), Loc(Loc), IfLoc(IfLoc) {}

  static bool classof(const PPDirective *D) { return D->getKind() == DK_EndIf; }

  SourceLocation Loc;
  SourceLocation IfLoc;
};

struct PPTree {
  PPDirectiveList Directives;
};

class PPTreeConsumer {
public:
  virtual ~PPTreeConsumer() = default;

  virtual void endOfMainFile(const utils::PPTree *Tree) = 0;
};

class PPTreeBuilder {
public:
  PPTreeBuilder(PPTreeConsumer *Callback, Preprocessor *PP,
                const SourceManager &SM, const LangOptions &LangOpts);

private:
  Preprocessor *PP;
  PPTreeConsumer *Callback;
  const SourceManager &SM;
  const LangOptions &LangOpts;
};

class PPMatchFinder;
class PPBoundNodesTreeBuilder;

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
}; // class Matcher

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

template <class Derived> class PPTreeVisitor {
public:
  void visit(const PPTree *Tree) { visitDirectives(Tree->Directives); }

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

  void visitDirectives(const PPDirectiveList &List);
};

template <class Derived>
void PPTreeVisitor<Derived>::visitDirectives(const PPDirectiveList &List) {
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
      visitDirectives(If->Directives);
      break;
    }
    case PPDirective::DK_Else: {
      const PPElse *Else = dyn_cast<PPElse>(Directive);
      getDerived().visitElse(Else);
      visitDirectives(Else->Directives);
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
      visitDirectives(IfDef->Directives);
      break;
    }
    case PPDirective::DK_IfNotDef: {
      const PPIfNotDef *IfNotDef = dyn_cast<PPIfNotDef>(Directive);
      getDerived().visitIfNotDef(IfNotDef);
      visitDirectives(IfNotDef->Directives);
      break;
    }
    case PPDirective::DK_ElseIfDef: {
      const PPElseIfDef *ElseIfDef = dyn_cast<PPElseIfDef>(Directive);
      getDerived().visitElseIfDef(ElseIfDef);
      visitDirectives(ElseIfDef->Directives);
      break;
    }
    case PPDirective::DK_ElseIfNotDef: {
      const PPElseIfNotDef *ElseIfNotDef = dyn_cast<PPElseIfNotDef>(Directive);
      getDerived().visitElseIfNotDef(ElseIfNotDef);
      visitDirectives(ElseIfNotDef->Directives);
      break;
    }
    case PPDirective::DK_EndIf:
      getDerived().visitEndIf(dyn_cast<PPEndIf>(Directive));
      break;
    }
  }
}

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

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_PPTREE_H
