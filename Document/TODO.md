TODO
---

[Optimization]
* Minimize regular expression (take advantage of algebraic laws!!)
  1. (L*)*   -> (L*)
  2. LM|LN   -> L(?:M|N)
  3. MP|NP   -> (?:M|N)P
  4. L|L     -> L
  5. ()*     -> ()
  6. L+      -> LL*
  7. L?      -> (?:L|)
* Minimize NFA? not rewarding.
* Build DFA
* Minimize DFA
  * table-filling

[Load Test]
* Create a 100mb int array in memory, compare
  * plain memory scan: how long to add them all?
  * regex matching: how long to find some special numbers?
* [web] Gather 1000 large html pages into memory, compare
  * plain memory scan
  * regex counting: how many 'br' tags
  * regex capturing: collect all texts in 'title'
* [lex] Tokenize a C program with more then 10k lines.
* [lex] Tokenize 100+ C programs with ~1k lines.

[Resource & Misc]
* Memory usage & management?
* Compute model?
* Input model: just an array of memory blocks
* Regex error handling

[Feature]
* Support character group: [a-z]
* Support regex options:
  * i: ignore case
  * m: multi-line
  * s: single-line
  * n: explicit capture

* Test Plan (correctness, performance)
