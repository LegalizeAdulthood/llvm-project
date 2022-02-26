.. title:: clang-tidy - modernize-prefer-scoped-enum

modernize-prefer-scoped-enum
============================

Finds unscoped enums that can be replaced with scoped enums.  To be
considered, enumerations must be used as whole expressions and not used
as subexpressions where they are implicitly converted to integers.  The
enumerations are examined for common a common prefix that can be stripped.
If stripping the common prefix would result in an invalid identifier,
then the prefix is not stripped.

Examples:

.. code-block:: c++

  enum Foo {
    FOO_ONE,
    FOO_TWO
  };

  void f(int foo) {
    switch (foo) {
    case FOO_ONE:
      g(1);
      break;

    case FOO_TWO:
      g(2);
      break;
    }
  }

  void g() {
    f(FOO_ONE);
    f(FOO_TWO);
  }

becomes

.. code-block:: c++

  enum class Foo {
    ONE,
    TWO
  };

  void f(Foo foo) {
    switch (foo) {
    case Foo::ONE:
      g(1);
      break;

    case Foo::TWO:
      g(2);
      break;
    }
  }

  void g() {
    f(Foo::ONE);
    f(Foo::TWO);
  }
