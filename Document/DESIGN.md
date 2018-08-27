Regular Expression Engine
===

Why is a regex engine useful?
---

0. It (often) efficiently implements a descriptive regular-expression notation.
   Essentially a compiler of regular-expression.
   * It can answer whether a string matches a pattern.
   * It can describe complex patterns, with the strong expression capability
     provided by regular(?) grammer.
   * By interpreting the structure of a string, it can extract the interesting
     part from the string. For example, the username field in an email address,
     after it verifies that the string is an email address. In compiler
     construction, it can also be used to build a fast lexer, with a few lines
     of code.
   * To extract, edit, replace, or delete text substrings.
1. It can be a convenient "recognizer" development tool. Fast trial-and-error
   without writing any code, just modify the regular-expression.

Examples
---

* Keyword search in a giant document: grep.
* Tokenization and lexical analysis: lex.

The elements of regex engine
---

* Q1: Does string S matches pattern P?
* Q2: What are the sub-strings in string S matches sub-parts in pattern P?
* Q3: What are the sub-strings S[] in a string matches pattern P?
* Q4: What are the sub-strings S[] in a string matches pattern P, if P is changed
    in a non-destructive way?

Design
---

* Alphabet
  * charset: ASCII, Unicode(utf, unicode character class, etc.)
  * single: a b c
  * set: [a-z] \w
* Pattern
  * concat: ab
  * alter: a|b
    * try in order
    * backtracking
  * repeat: ab*
    * backtracking
    * repeat range
      * equal: a{2}b
      * no less than: a{2,}b
      * no more than: a{,2}b
      * inclusive within: a{2,4}b
    * flavor
      * greedy (default)
      * reluctant: a*?b, a{1,2}?b
      * possessive: use atomic group
  * group: a(bc)
    * capture capability
      * nesting capture: a(b(c))
      * back reference: a(bc)\1
    * non-capture notation: a(?:bc)
       * not occupy group id.
    * non-consumption capability
      * lookaround assertion: a(?=yes)y a(?<no)n
    * atomic capability (disable inner backtracking)
      * disable backtracking in repeat: (?>a*)ab
      * disable backtracking in alter: (?>a|ab)b
* Match
  * return the first match
* Algorithm
  * regex => epsilon-NFA
  * epsilon-NFA => DFA
  * simulate epsilon-NFA
  * simulate DFA

* Advanced features
  * [group] matching configuration.
  * [alter] empty match (e.g. "(a|)")

* Easy implement features
  * [group, capturing] name property: a(?<lang>c|cpp)
  * [alter] conditional or by expression: a(?(b)bob|peter)
  * [alter] conditional or by named group: a(?(name)human|robot)

Implementation
---

* Input model - array of memory blocks

> ASCII Alphabet + Regex

0. Represent epsilon-NFA, DFA
```c++
/*
 * Represent epsilon-NFA
 */

struct NfaState {
	// next(c)? -> NfaState*[]
	int next[128];
	// terminal? -> bool
	bool terminal;
};
struct Nfa {
	// next(c, s[])? -> NfaState*[]
	// terminal(s[])? -> bool
};

/*
 * Represent DFA
 */

// sparse
struct DfaState {
	// next(c)? -> DfaState*
	// terminal? -> bool
};
struct Dfa {
	// start? -> DfaState*
	// vector<DfaState>
};

// dense
struct Dfa {
	// next(c, s)? -> int
	int next[CharCount() * StateCount()];
	// terminal(s)? -> bool
	bool terminal[StateCount()];
};

```
1. Build epsilon-NFA from regular expression
2. Simulate epsilon-NFA



> NFA with backtracking? or DFA with more states? How this two things convert between each other?
> How to compile pattern string into a "runnable machine"?

> Rpeatition behavior
  * Greedy.
  * Reluctant.
  * Possessive.


* Program architecture
  * NFA based / DFA based / VM based
* More features
  * \w, \c, ...
  * Named group, recursive named group.
  * Capture history.
  * Nested capture.
  * Atomic group.
  * Greedy vs reluctant vs possessive.
* More advanced features
  * JIT capable.
  * Online parsing.
  * Enormous regular expression.
