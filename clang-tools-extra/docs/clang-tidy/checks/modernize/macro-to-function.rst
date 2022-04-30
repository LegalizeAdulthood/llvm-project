.. title:: clang-tidy - modernize-macro-to-function

modernize-macro-to-function
===========================

Replaces function-like macros that compute a value from their arguments
with a template function with a deduced return type.
Using a template function ensures that macro call sites with arguments
of different types continue to compile.  A template argument is created
for each macro argument, allowing template type deduction to match all
the types of actual macro arguments.  Using a deduced return type ensures
that the return type is appropriate for the type of the arguments.

Deduced return types for template functions are a C++14 addition, so
this check is only enabled when C++14 or later language standards are
in effect.

This check can be used to enforce the C++ core guideline `ES.31:
Don't use macros for constants or "functions"
<https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#es31-dont-use-macros-for-constants-or-functions>`_,
within the constraints outlined below.

Potential macros for replacement must meet the following constraints:

- Macros must expand only to literal tokens or simple expressions of
  literal tokens and the macro arguments.
- Macros must not be defined within a conditional compilation block.
  (Conditional include guards are exempt from this constraint.)
- Macros must not be used in any conditional preprocessing directive.
- Macros must not be undefined.
- Macros must be defined only once.

Examples:

.. code-block:: c++

  #define SQUARE(x) ((x)*(x))
  #define PYTHAG(x, y) (sqrt((x)*(x) + (y)*(y)))

becomes

.. code-block:: c++

  template <typename T> auto SQUARE(T x) { return ((x)*(x)); }
  template <typename T, typename T2> auto PYTHAG(T x, T2 y) { return (sqrt((x)*(x) + (y)*(y))); }
