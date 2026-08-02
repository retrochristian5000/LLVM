.. If you want to modify sections/contents permanently, you should modify both
   ReleaseNotes.rst and ReleaseNotesTemplate.txt.

====================================================
Extra Clang Tools |release| |ReleaseNotesTitle|
====================================================

.. contents::
   :local:
   :depth: 3

Written by the `LLVM Team <https://llvm.org/>`_

.. only:: PreRelease

  .. warning::
     These are in-progress notes for the upcoming Extra Clang Tools |version| release.
     Release notes for previous releases can be found on
     `the Download Page <https://releases.llvm.org/download.html>`_.

Introduction
============

This document contains the release notes for the Extra Clang Tools, part of the
Clang release |release|. Here we describe the status of the Extra Clang Tools in
some detail, including major improvements from the previous release and new
feature work. All LLVM releases may be downloaded from the `LLVM releases web
site <https://llvm.org/releases/>`_.

For more information about Clang or LLVM, including information about
the latest release, please see the `Clang Web Site <https://clang.llvm.org>`_ or
the `LLVM Web Site <https://llvm.org>`_.

Note that if you are reading this file from a Git checkout or the
main Clang web page, this document applies to the *next* release, not
the current one. To see the release notes for a specific release, please
see the `releases page <https://llvm.org/releases/>`_.

What's New in Extra Clang Tools |release|?
==========================================

Some of the major new features and improvements to Extra Clang Tools are listed
here. Generic improvements to Extra Clang Tools as a whole or to its underlying
infrastructure are described first, followed by tool-specific sections.

Major New Features
------------------

Potentially Breaking Changes
----------------------------

Improvements to clangd
----------------------

Inlay hints
^^^^^^^^^^^

Diagnostics
^^^^^^^^^^^

Semantic Highlighting
^^^^^^^^^^^^^^^^^^^^^

Compile flags
^^^^^^^^^^^^^

Hover
^^^^^

Code completion
^^^^^^^^^^^^^^^

Code actions
^^^^^^^^^^^^

Signature help
^^^^^^^^^^^^^^

Cross-references
^^^^^^^^^^^^^^^^

Objective-C
^^^^^^^^^^^

Miscellaneous
^^^^^^^^^^^^^

Improvements to clang-doc
-------------------------

Improvements to clang-query
---------------------------

Improvements to clang-tidy
--------------------------

New checks
^^^^^^^^^^

- New :doc:`bugprone-assignment-in-selection-statement
  <clang-tidy/checks/bugprone/assignment-in-selection-statement>` check.

  Finds assignments within selection statements.

- New :doc:`bugprone-missing-end-comparison
  <clang-tidy/checks/bugprone/missing-end-comparison>` check.

  Finds instances where the result of a standard algorithm is used in a Boolean
  context without being compared to the end iterator.

- New :doc:`bugprone-unsafe-to-allow-exceptions
  <clang-tidy/checks/bugprone/unsafe-to-allow-exceptions>` check.

  Finds functions where throwing exceptions is unsafe but the function is still
  marked as potentially throwing.

- New :doc:`llvm-formatv-string
  <clang-tidy/checks/llvm/formatv-string>` check.

  Validates ``llvm::formatv`` format strings against the provided arguments,
  diagnosing mismatched argument counts, unused arguments, and mixed index styles.

- New :doc:`llvm-redundant-casting
  <clang-tidy/checks/llvm/redundant-casting>` check.

  Points out uses of ``cast<>``, ``dyn_cast<>`` and their ``or_null`` variants
  that are unnecessary because the argument already is of the target type, or a
  derived type thereof. Also does similar analysis for calls to ``isa<>`` that
  always return ``true``.

- New :doc:`llvm-type-switch-case-types
  <clang-tidy/checks/llvm/type-switch-case-types>` check.

  Finds ``llvm::TypeSwitch::Case`` calls with redundant explicit template
  arguments that can be inferred from the lambda parameter type.

- New :doc:`llvm-use-vector-utils
  <clang-tidy/checks/llvm/use-vector-utils>` check.

  Finds calls to ``llvm::to_vector(llvm::map_range(...))`` and
  ``llvm::to_vector(llvm::make_filter_range(...))`` that can be replaced with
  ``llvm::map_to_vector`` and ``llvm::filter_to_vector``.

- New :doc:`misc-static-initialization-cycle
  <clang-tidy/checks/misc/static-initialization-cycle>` check.

  Finds cyclical initialization of static variables.

- New :doc:`modernize-use-std-bit
  <clang-tidy/checks/modernize/use-std-bit>` check.

  Finds common idioms which can be replaced by standard functions from the
  ``<bit>`` C++20 header.

- New :doc:`modernize-use-string-view
  <clang-tidy/checks/modernize/use-string-view>` check.

  Looks for functions returning ``std::[w|u8|u16|u32]string`` and suggests to
  change it to ``std::[...]string_view`` for performance reasons if possible.

- New :doc:`modernize-use-structured-binding
  <clang-tidy/checks/modernize/use-structured-binding>` check.

  Finds places where structured bindings could be used to decompose pairs and
  suggests replacing them.

- New :doc:`performance-expensive-value-or
  <clang-tidy/checks/performance/expensive-value-or>` check.

  Finds calls to ``value_or`` (and alternative spellings ``valueOr``,
  ``ValueOr``) on optional types where the return type is expensive to copy.

- New :doc:`performance-string-view-conversions
  <clang-tidy/checks/performance/string-view-conversions>` check.

  Finds and removes redundant conversions from ``std::[w|u8|u16|u32]string_view`` to
  ``std::[...]string`` in call expressions expecting ``std::[...]string_view``.

- New :doc:`performance-use-std-move
  <clang-tidy/checks/performance/use-std-move>` check.

  Suggests insertion of ``std::move(...)`` to turn copy assignment operator
  calls into move assignment ones, when deemed valid and profitable.

- New :doc:`readability-redundant-lambda-parameter-list
  <clang-tidy/checks/readability/redundant-lambda-parameter-list>` check.

  Finds lambda expressions with a redundant empty parameter list and removes it.

- New :doc:`readability-redundant-nested-if
  <clang-tidy/checks/readability/redundant-nested-if>` check.

  Finds nested ``if`` statements that can be merged into a single ``if`` by
  combining conditions with ``&&``.

- New :doc:`readability-redundant-qualified-alias
  <clang-tidy/checks/readability/redundant-qualified-alias>` check.

  Finds redundant identity type aliases that re-expose a qualified name and can
  be replaced with a ``using`` declaration.

- New :doc:`readability-redundant-zero-initializer
  <clang-tidy/checks/readability/redundant-zero-initializer>` check.

  Finds explicit zero initializers of arrays that can be replaced with empty
  braces.

- New :doc:`readability-trailing-comma
  <clang-tidy/checks/readability/trailing-comma>` check.

  Checks for presence or absence of trailing commas in enum definitions and
  initializer lists.

New check aliases
^^^^^^^^^^^^^^^^^

Changes in existing checks
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Improved :doc:`misc-redundant-expression
  <clang-tidy/checks/misc/redundant-expression>` by fixing false positives in
  nested expressions involving different macros or a mix of macro and
  non-macro operands.

- Improved :doc:`readability-named-parameter
  <clang-tidy/checks/readability/named-parameter>` check by ignoring
  standard tag types (e.g. ``std::in_place_t``, ``std::allocator_arg_t``,
  ``std::nothrow_t``, iterator tags, lock tags, etc.) that are used
  exclusively for overload resolution. Added the :option:`IgnoredTypes`
  option to allow customizing the set of ignored types.

- Improved :doc:`readability-use-std-min-max
  <clang-tidy/checks/readability/use-std-min-max>` check by fixing spurious
  trailing semicolons and lost comments when the ``if`` body has no braces.

Removed checks
^^^^^^^^^^^^^^

Miscellaneous
^^^^^^^^^^^^^

Improvements to include-fixer
-----------------------------

Improvements to clang-include-fixer
-----------------------------------

Improvements to modularize
--------------------------

Improvements to pp-trace
------------------------

Clang-tidy Visual Studio plugin
-------------------------------
