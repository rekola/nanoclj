# nanoclj

[![CI](https://github.com/rekola/nanoclj/workflows/Ubuntu-CI/badge.svg)]()
[![CI](https://github.com/rekola/nanoclj/workflows/VS17-CI/badge.svg)]()
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
| wezterm | Cannot get background color | OK | Buggy (as of 20230712) |
| xterm | No true color, inline image layout doesn't work | OK | |
| mlterm | Cannot get background color, inline image layout doesn't work | ? | Output flickers when updated |
| Black Box | Inline image layout doesn't work | ? | On HiDPI system the images are upscaled, and the terminal and the flatpak system use too much CPU time when idling. |
| GNOME Terminal | Sixels are not enabled by default | | |

![Plotting from nanoclj](https://user-images.githubusercontent.com/6755525/277504459-737b498e-005b-49ad-92b2-0917a1a10b7e.jpg "Plotting from nanoclj")
*The plot function returns a canvas, which can be converted to an image, and then saved with Image/save or modified using other functions in the Image namespace.*

As well as printing images in block mode like the plot function does, they can also be printed in inline mode which is shown in the following example:

![Inline images](https://user-images.githubusercontent.com/6755525/277514315-6b5f26a0-a1ab-4f66-95b7-4c976f288ff3.jpg "Inline images")

## Differences to Clojure:

- Characters are 32 bit and strings and char arrays are UTF-8 (count is O(n))
- Strings are sequences, and they are compared and sorted as such.
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
- libxml2
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
- BigInts, BigDecimals and Exotic numeric literals (e.g. 3N, 0.1M, hexadecimal floats)
- Persistent data structures
- Transient data structures
- Unchecked operations
- Autopromoting operations
- StructMaps
- Interfaces, Records, Protocols and Multi-methods
- Locals cleaning
- Multithreading, transactions and STM
- monitor-enter, monitor-exit, and locking
- Metadata reader macro, Threading macros (->, -->, some-> and some->>)
- Homogenous vectors (vector-of)
- Pre and post conditions for functions
- Readers, Writers, spit and slurp do not accept options such as :encoding or :append
- spit and slurp do not accept Writers or Readers
- Associative destructuring
- `*print-length*`, `*print-level*`, `*file*`, `*flush-on-newline*`, `*clojure-version*`, `*load-tests*`, `*print-meta*`
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
  - re-groups, re-matcher, re-seq, re-matches
  - hash-ordered-coll, hash-unordered-coll
  - assert-args
  - make-parents
  - with-open
  - with-meta, vary-meta, alter-meta!, reset-meta!
- clojure.core.async
  - thread-call
- clojure.core.reducers
- clojure.main
  - load-script
- clojure.string
  - split
  - replace, replace-first
- clojure.repl
  - dir
- clojure.test
  - deftest, set-test, with-test
- clojure.set
- clojure.data
- clojure.walk
- clojure.zip
- clojure.pprint
- clojure.spec
- ...and lots more...
