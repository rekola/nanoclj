# nanoclj

[![CI](https://github.com/rekola/nanoclj/workflows/CI/badge.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

## A Tiny Clojure Interpreter

This is a small Clojure language implementation written in C
language. It is based on TinyScheme which was based on MiniScheme.

## 2D Graphics

Since Clojure has no standard way for 2D drawing, a new system has
been created for nanoclj. The principle of minimality suggests that
there should only be a single way to output text, which means that
println must work with canvas. Therefore, an active canvas is bound to
`*out*` and it's the target of all canvas operations. For normal `*out*`
the canvas commands are ignored (apart from color and restore), but
one could imagine that they could be used to draw vector graphics on a
Tektronix terminal, or move-to could set the cursor position in a text
mode program. The canvas interface has been modeled after Cairo.

In GUI program the resulting canvas can be displayed in a REPL using
the GUI callbacks for output Writer. In terminal program sixels are
used to print the results of a function call when the function returns
a canvas or image. The terminal must, of course, has to support sixel
graphics and sixel support must be enabled. The following terminals
have been tested:

| Terminal | Graphics | Mouse | Notes |
| - | - | - | - |
| foot | OK | OK | Wayland only |
| wezterm | OK | OK | Buggy (as of 20230712) |
| mlterm | OK | ? | Output flickers when updated. |
| xterm | Sixels work, but no True color | OK | |
| Black Box | OK | ? | On HiDPI system the images are upscaled, and the terminal and the flatpak system use too much CPU time when idling. |
| GNOME Terminal | True color works, but sixel support is not enabled. | | |

![Plotting from nanoclj](https://user-images.githubusercontent.com/6755525/262003070-b5eac109-f1cc-4071-ad7b-a1e5d107a1d9.jpeg "Plotting from nanoclj")
*The plot function returns an image which can then be saved with Image/save or modified using other functions in the Image namespace.*

## Differences to Clojure:

- Characters are 32 bit and strings and char arrays are UTF-8 (count is O(n))
- Strings are compared and sorted by the sequences of their UTF-32 codepoints.
- By default 32 bit integers are used, since 64 bit integers don't fit in the NaN boxing.
- List count has complexity of O(n)
- Vectors support O(1) append, but update is O(n)
- Macros use the TinyScheme syntax
- Tail-call optimization
- sort is not stable and doesn't throw on mismatching types
- License is BSD 2-Clause License instead of EPL
- No 32-bit floating point numbers or small integers
- Symbols are interned and cannot have metadata
- Primitives such as doubles and 32 bit integers are passed by value, and are in effect, interned
- Strings are not interned (identical? "abc" "abc") ;=> false
- `rationalize` is exact and Ratios use the smallest suitable integer type for numerator and denominator
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
- Type names try to imitate Java and Clojure types when possible (type \a) ;=> java.lang.Character

## Dependencies

- linenoise (included, utf8 support added)
- stb_image, stb_image_resize, stb_image_write (included)
- dr_wav (included)
- ggml (included, experimental)
- pcre2
- cairo
- libsixel
- utf8proc
- shapelib

## Missing functionality

- Custom types
- Transducers
- Refs, Agents, Atoms, Validators
- Queues, Arrays
- Reader Conditionals
- BigInts, BigDecimals and Exotic numeric literals (e.g. NrXXX, 3N, 0.1M, hexadecimal floats)
- Persistent data structures
- Transient data structures
- Unchecked operations
- Autopromoting operations
- StructMaps
- Interfaces, Records, Protocols and Multi-methods
- Locals cleaning
- Multithreading, transactions and STM
- monitor-enter, monitor-exit, and locking
- Threading macros (->, -->, some-> and some->>)
- Homogenous vectors (vector-of)
- Pre and post conditions for functions
- Readers, Writers, spit and slurp do not accept options such as :encoding or :append
- spit and slurp do not accept Writers or Readers
- `*print-length*`, `*print-level*`, `*file*`, `*flush-on-newline*`, `*clojure-version*`, `*load-tests*`
- Missing core functions and macros
  - doseq, for, dotimes
  - bit-and-not
  - sort-by, sorted-set-by, sorted-map, hash-set, hash-map
  - update-in, merge-with
  - cast, doto
  - parse-long, parse-double, parse-uuid
  - map-indexed, mapcat, zipmap, lazy-cat
  - partition-by
  - juxt
  - memoize
  - group-by
  - condp
  - when-let, letfn, if-let, if-some
  - reduced, reduced?
  - with-local-vars, var-set, find-var, alter-var-root, declare, binding
  - as-url, resource
  - sequence
  - make-hierarchy, ancestors, supers, bases
  - bound?
  - random-uuid
  - bounded-count
  - deftest, set-test, with-test
  - re-groups, re-matcher, re-seq, re-matches
  - hash-ordered-coll, hash-unordered-coll
  - assert-args
  - make-parents
  - with-open
- clojure.core.reducers
- clojure.main
  - load-script
- clojure.string
  - upper-case
  - capitalize
  - split
  - replace, replace-first
- clojure.set
- clojure.data
- clojure.xml
- clojure.walk
- clojure.zip
- clojure.pprint
- clojure.spec
- clojure.test
- ...and lots more...
