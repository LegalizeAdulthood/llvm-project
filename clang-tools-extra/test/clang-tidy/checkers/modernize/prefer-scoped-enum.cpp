// RUN: %check_clang_tidy %s modernize-prefer-scoped-enum %t

enum Foo {
  // CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Prefer a scoped enum to the unscoped enum 'Foo'
  // CHECK-FIXES: enum class Foo {
  FOO_ONE,
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Prefer a scoped enum to the unscoped enum 'Foo'
  // CHECK-FIXES: ONE,
  FOO_TWO
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: Prefer a scoped enum to the unscoped enum 'Foo'
  // CHECK-FIXES: TWO
};

enum class Bar {
  ONE,
  TWO
};

extern void g(int x);

void f(int foo) {
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer a scoped enum to the unscoped enum 'Foo'
  // CHECK-FIXES: void f(Foo foo) {
  switch (foo) {
  case FOO_ONE:
    // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer a scoped enum to the unscoped enum 'Foo'
    // CHECK-FIXES: Foo::ONE:
    g(1);
    break;

  case FOO_TWO:
    // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: Prefer a scoped enum to the unscoped enum 'Foo'
    // CHECK-FIXES: Foo::TWO:
    g(2);
    break;
  }
}

void h() {
  f(FOO_ONE);
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Prefer a scoped enum to the unscoped enum 'Foo'
  // CHECK-FIXES: Foo::ONE:
  f(FOO_TWO);
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Prefer a scoped enum to the unscoped enum 'Foo'
  // CHECK-FIXES: Foo::TWO:
}

void gn(Bar bar) {
  switch (bar) {
  case Bar::ONE:
    g(1);
    break;

  case Bar::TWO:
    g(2);
    break;
  }
}

void fn() {
  gn(Bar::ONE);
  gn(Bar::TWO);
}
