// RUN: %check_clang_tidy -std=c++17 -check-suffix=,EXPLICIT %s modernize-loop-convert %t -- -config='{CheckOptions: {modernize-loop-convert.UseAuto: false}}'
// RUN: %clang_cc1 -std=c++17 -fsyntax-only %t.cpp
// RUN: %check_clang_tidy -std=c++17 -check-suffix=,EXPLICIT %s modernize-loop-convert %t-reverse -- -config='{CheckOptions: {modernize-loop-convert.UseAuto: false}}' -- -DREVERSE_DECLARATIONS
// RUN: %clang_cc1 -std=c++17 -fsyntax-only %t-reverse.cpp
// RUN: %check_clang_tidy -std=c++17 -check-suffix=,AUTO %s modernize-loop-convert %t-default
// RUN: %check_clang_tidy -std=c++17 -check-suffix=,AUTO %s modernize-loop-convert %t-auto -- -config='{CheckOptions: {modernize-loop-convert.UseAuto: true}}'

// CHECK-FIXES: namespace std {
namespace std {
template <class Char> struct basic_string {
  void modify();
  void read() const;
  ~basic_string();
};
using string = basic_string<char>;

template <class T> struct vector {
  using iterator = T *;
  using const_iterator = const T *;
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  unsigned size() const;
  T &operator[](unsigned);
  const T &operator[](unsigned) const;
};

template <class First, class Second> struct pair {
  First first;
  Second second;
};
template <class Key, class Value> struct map {
  using value_type = pair<const Key, Value>;
  using iterator = value_type *;
  iterator begin();
  iterator end();
};
} // namespace std

#ifdef REVERSE_DECLARATIONS
std::vector<std::basic_string<char>> *First;
#endif

// These share a canonical specialization. Recover each declaration's sugar,
// regardless of which spelling instantiated the specialization first.
void strings(std::vector<std::string> &Strings,
             std::vector<std::basic_string<char>> &Expanded) {
  for (unsigned I = 0; I < Strings.size(); ++I)
    Strings[I].modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (std::string &String : Strings)
  // CHECK-FIXES-AUTO: for (auto & String : Strings)

  for (auto It = Expanded.begin(); It != Expanded.end(); ++It)
    It->modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (std::basic_string<char> &It : Expanded)
  // CHECK-FIXES-AUTO: for (auto & It : Expanded)

  for (auto It = Strings.begin(); It != Strings.end(); ++It)
    It->modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (std::string &String : Strings)
  // CHECK-FIXES-AUTO: for (auto & String : Strings)

  for (unsigned I = 0; I < Expanded.size(); ++I)
    Expanded[I].modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (std::basic_string<char> &I : Expanded)
  // CHECK-FIXES-AUTO: for (auto & I : Expanded)
}

namespace user {
struct Item {
  void modify();
  void read() const;
  ~Item();
};
using Alias = Item;
using Items = std::vector<Alias>;
} // namespace user

// Here the expanded spelling comes first.
void aliases(std::vector<user::Item> &Items,
             std::vector<user::Alias> &Aliases, user::Items *Pointer) {
  for (auto It = Items.begin(); It != Items.end(); ++It)
    It->modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Item &Item : Items)
  // CHECK-FIXES-AUTO: for (auto & Item : Items)

  for (unsigned I = 0; I < Aliases.size(); ++I)
    Aliases[I].modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias &Aliase : Aliases)
  // CHECK-FIXES-AUTO: for (auto & Aliase : Aliases)

  for (auto It = Pointer->begin(); It != Pointer->end(); ++It)
    It->modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias &{{[A-Za-z]+}} : *Pointer)
  // CHECK-FIXES-AUTO: for (auto & {{[A-Za-z]+}} : *Pointer)
}

void qualifiers(const std::vector<std::string> &Strings,
                std::vector<user::Alias *> &Pointers) {
  for (unsigned I = 0; I < Strings.size(); ++I)
    Strings[I].read();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (const std::string &String : Strings)
  // CHECK-FIXES-AUTO: for (const auto & String : Strings)

  for (auto It = Pointers.begin(); It != Pointers.end(); ++It)
    (*It)->modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias *&Pointer : Pointers)
  // CHECK-FIXES-AUTO: for (auto & Pointer : Pointers)
}

void arrays() {
  using Number = int;
  Number Numbers[3];
  for (int I = 0; I < 3; ++I)
    Numbers[I] = 0;
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (Number &Number : Numbers)
  // CHECK-FIXES-AUTO: for (int & Number : Numbers)

  const Number Constants[3] = {};
  for (int I = 0; I < 3; ++I)
    (void)(Constants[I] + 1);
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (Number Constant : Constants)
  // CHECK-FIXES-AUTO: for (int Constant : Constants)

  user::Alias Objects[3];
  for (int I = 0; I < 3; ++I)
    Objects[I].modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias &Object : Objects)
  // CHECK-FIXES-AUTO: for (auto & Object : Objects)

  int Matrix[3][4];
  for (int I = 0; I < 3; ++I)
    Matrix[I][0] = 0;
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (int (&I)[4] : Matrix)
  // CHECK-FIXES-AUTO: for (auto & I : Matrix)

  void (*Functions[3])();
  for (int I = 0; I < 3; ++I)
    Functions[I]();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (void (*&Function)() : Functions)
  // CHECK-FIXES-AUTO: for (auto & Function : Functions)
}

void nested(std::vector<std::vector<std::string>> &Vectors,
            std::map<int, user::Alias> &Map) {
  for (auto It = Vectors.begin(); It != Vectors.end(); ++It)
    (void)It->size();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (std::vector<std::string> &Vector : Vectors)
  // CHECK-FIXES-AUTO: for (auto & Vector : Vectors)

  for (auto It = Map.begin(); It != Map.end(); ++It)
    It->second.modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (std::pair<const int, user::Alias> &It : Map)
  // CHECK-FIXES-AUTO: for (auto & It : Map)
}

void bodyAliases(std::vector<user::Alias> &Objects) {
  for (auto It = Objects.begin(); It != Objects.end(); ++It) {
    const user::Alias &Object = *It;
    Object.read();
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias &Object : Objects) {
  // CHECK-FIXES-AUTO: for (auto & Object : Objects) {

  for (auto It = Objects.begin(); It != Objects.end(); ++It) {
    user::Alias Object = *It;
    Object.modify();
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias Object : Objects) {
  // CHECK-FIXES-AUTO: for (auto Object : Objects) {
}

void anonymous() {
  struct { int Value; } Objects[3];
  for (int I = 0; I < 3; ++I)
    Objects[I].Value = 0;
  // CHECK-MESSAGES-AUTO: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (int I = 0; I < 3; ++I)
  // CHECK-FIXES-AUTO: for (auto & Object : Objects)

  enum { Zero } Values[3] = {};
  for (int I = 0; I < 3; ++I)
    (void)(Values[I] == Zero);
  // CHECK-MESSAGES-AUTO: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (int I = 0; I < 3; ++I)
  // CHECK-FIXES-AUTO: for (auto & Value : Values)
}

template <class T> struct Range {
  struct iterator {
    T &operator*();
    iterator &operator++();
    bool operator!=(const iterator &) const;
  };
  iterator begin();
  iterator end();
};

template <class T> struct ValueRange {
  struct iterator {
    T operator*();
    iterator &operator++();
    bool operator!=(const iterator &) const;
  };
  iterator begin();
  iterator end();
};

void iterators(Range<user::Alias> &Objects,
               ValueRange<user::Alias> &Values) {
  for (auto It = Objects.begin(); It != Objects.end(); ++It)
    (*It).modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias &Object : Objects)
  // CHECK-FIXES-AUTO: for (auto & Object : Objects)

  for (auto It = Values.begin(); It != Values.end(); ++It)
    (*It).modify();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (user::Alias &&Value : Values)
  // CHECK-FIXES-AUTO: for (auto && Value : Values)
}

void constValues(ValueRange<const user::Alias> &Values) {
  for (auto It = Values.begin(); It != Values.end(); ++It)
    (*It).read();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (const user::Alias &&Value : Values)
  // CHECK-FIXES-AUTO: for (auto && Value : Values)
}

void bindings(std::map<int, user::Alias> &Map) {
  for (auto It = Map.begin(); It != Map.end(); ++It) {
    auto &[Key, Value] = *It;
    Value.modify();
  }
  // CHECK-MESSAGES-AUTO: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (auto It = Map.begin(); It != Map.end(); ++It) {
  // CHECK-FIXES-AUTO: for (auto [Key, Value] : Map) {
}

class Hidden {
  struct Element {
    void modify();
  };

public:
  Element *begin();
  Element *end();
};

void hidden(Hidden &Objects) {
  for (auto It = Objects.begin(); It != Objects.end(); ++It)
    It->modify();
  // CHECK-MESSAGES-AUTO: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (auto It = Objects.begin(); It != Objects.end(); ++It)
  // CHECK-FIXES-AUTO: for (auto & Object : Objects)
}

class HiddenAlias {
  using Alias = user::Item;

public:
  std::vector<Alias> Objects;
};

void hiddenAlias(HiddenAlias &Container) {
  for (unsigned I = 0; I < Container.Objects.size(); ++I)
    Container.Objects[I].modify();
  // CHECK-MESSAGES-AUTO: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES-EXPLICIT: for (unsigned I = 0; I < Container.Objects.size(); ++I)
  // CHECK-FIXES-AUTO: for (auto & Object : Container.Objects)
}
