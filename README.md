# nanoclj

## A Tiny Clojure Interpreter

- This is small `Clojure` language implementation written in `C` language.
- Based on TinyScheme R7, which is based on TinyScheme, which is based on MiniScheme
- It does not attempt to be a real Clojure but a real Lisp with Clojure syntax. I.e. lists are actual Lisp lists built from cons cells
- Not yet complete nor fast
- Somewhat similar to Janet, but has Unicode out of the box. Similar to Planck but doesn't use Javascript or ClojureScript.

## Possible applications

- as a tiny engine for running plugins and other extensions in your `C/C++` program (suppose you want to change some part of logic of your program without recompiling it every time).
- as a sandbox to learn the language (as it supports most common features except those which will make implementation not "nano")
- as utility script-running tool, even to create `CGI-scripts` for web-server

## Differences to Clojure:

- Characters are 32 bit and strings and char arrays are UTF-8 (length is O(n))
- Strings are compared by their UTF-8 representation
- By default 32 bit integers are used, since 64 bit integers don't fit in the NaN boxing.
- (cons 1 2) is allowed
- List length has complexity of O(n)
- Macros use Scheme syntax
- Tail call optimization
- sort is not stable and cannot throw on type mismatch
- Namespace definition uses TinyScheme syntax
- License is BSD 2-Clause License instead of EPL
- No type inheritance
- No 32 bit floating point numbers or small integers
- (identical 'a 'a) ; => true
- All primitives are, in effect, interned (e.g. doubles and 32 bit integers)
- Strings are not interned (identical? "abc" "abc") ;=> false
- Ratios use smallest possible integer type for numerator and denominator
- No chunked lazy sequences

## Differences to (Tiny)Scheme:

- Ports have been renamed to Readers and Writers, environments to namespaces, promises to delays
- Literals and function names have been changed
- Lambdas changed from (lambda (x) x) to (fn [x] x)
- nil is false and not the same as empty list
- Callback ports have been added for printing into a GUI instead of stdout
- Metadata has been added
- Vector data is stored as an array of primitive values, not as an array of cells
- Lists can be built from sequences: (cons 1 "Hello")
- NaN-boxing has been added
- REPL output is colored by type
- Lazy sequences can be printed by the REPL and used with apply
- Integer overflows and underflows are detected
- Unnecessary functionality has been removed: continuations, Plists, type checks

## Types

- Types are first class values and can be invoked to construct new values
- Type names try to imitate Java and Clojure types where possible (type \a) ;=> java.lang.Character

## Missing functionality

- Custom types
- Transducers
- Exception handling
- Regular expressions
- Vars, Refs, Agents and Atoms
- Queues
- Arrays
- Tagged Literals (e.g. #uuid and #inst)
- Reader Conditionals
- Namespace qualifiers for keywords
- Numeric literals other than decimal (e.g. 0xff, 2r0, 3N, 0.1M, 01)
- Persistent data structures
- BigInts and BigDecimals
- Unchecked operations
- Autopromoting operations
- StructMaps
- Multithreading and transactions
- Threading macros (->, -->, some-> and some->>)
- Classes (class, class?, cast)
- *print-length*, *print-level*
- Missing core functions and macros
  - doseq, for, dotimes
  - line-seq
  - bit-and-not, unsigned-bit-shift-right
  - file, file-seq
  - sort-by
  - letfn
  - keep
  - mapv, filterv
  - sorted-set-by, sorted-map, hash-set
  - *clojure-version*
  - update, update-in, merge, get-in, disj
  - name
  - lazy-cat, realized?
  - mapcat
  - nthrest, nthnext, nfirst
  - parse-long, parse-double, parse-uuid
  - distinct?
  - not-every?, not-any?
  - map-indexed
  - partition-by
  - remove
  - merge-with
  - juxt
  - cycle
  - rseq
  - find
  - keys
  - flatten
  - zipmap
  - memoize
  - partition
  - group-by
  - condp
  - doto
  - when-not, when-let
  - if-not
  - if-let
  - realized?
- clojure.string
  - upper-case
  - clojure.string/capitalize
  - clojure.string/split
  - clojure.string/replace
  - trim, trimr, trim-newline
- ...and lots more...

## Dependencies

- linenoise (included)
- utf8proc
