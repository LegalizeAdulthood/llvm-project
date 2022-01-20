// RUN: %check_clang_tidy %s bugprone-macro-condition %t

#define USE_FOO 0
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: Macro 'USE_FOO' defined here with a value and checked for definition

#if defined(USE_FOO)
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: Macro 'USE_FOO' defined with a value and checked here for definition
void f()
{
  extern void foo();
  foo();
}
#endif

#if !defined(USE_FOO)
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Macro 'USE_FOO' defined with a value and checked here for definition
void f2()
{
  extern void notFoo();
  notFoo();
}
#endif

#ifdef USE_FOO
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: Macro 'USE_FOO' defined with a value and checked here for definition
void f3()
{
  extern void foo();
  foo();
}
#endif

#ifndef USE_FOO
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: Macro 'USE_FOO' defined with a value and checked here for definition
void f4()
{
  extern void notFoo();
  notFoo();
}
#endif

#if 0
#elif defined(USE_FOO)
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: Macro 'USE_FOO' defined with a value and checked here for definition
void f5()
{
  extern void foo();
  foo();
}
#endif
