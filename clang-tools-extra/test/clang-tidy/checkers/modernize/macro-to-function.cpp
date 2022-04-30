// RUN: %check_clang_tidy -std=c++14-or-later %s modernize-macro-to-function %t

#define FOO __foo

#define F(x_) int y = x_
#define G(x_) ((x_)/100)
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: replace macro with template function [modernize-macro-to-function]
// CHECK-MESSAGES: :[[@LINE-2]]:9: warning: macro 'G' defines an expression of its arguments; prefer an inline function instead [modernize-macro-to-function]
// CHECK-FIXES: template <typename T> auto G(T x_) { return ((x_)/100); }

#define G2(x_, y_) \
    (x_ + 1)/(y_ - 1)
// CHECK-MESSAGES: :[[@LINE-2]]:1: warning: replace macro with template function [modernize-macro-to-function]
// CHECK-MESSAGES: :[[@LINE-3]]:9: warning: macro 'G2' defines an expression of its arguments; prefer an inline function instead [modernize-macro-to-function]
// CHECK-FIXES: template <typename T, typename T2> auto G2(T x_, T2 y_) { return (x_ + 1)/(y_ - 1); }

// Not simple value expressions
#define H(x_, y_) std::complex<double>(x_, y_)
#define SC(t_, val_) static_cast<t_>(val_)
