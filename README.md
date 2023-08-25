# nanoclj

## A Tiny Clojure Interpreter

This is small Clojure language implementation written in C language.
It is based on TinyScheme R7, which is based on TinyScheme,
which is based on MiniScheme. nanoclj does not attempt to be a real
Clojure but a real Lisp with Clojure syntax. I.e. lists are actual
Lisp lists built from cons cells. It is not yet complete nor fast. It
is somewhat similar to Janet, but has Unicode out of the box. Similar
to Planck but doesn't use Javascript or ClojureScript.

## Possible applications

- as a tiny engine for running plugins and other extensions in your C/C++ program (suppose you want to change some part of logic of your program without recompiling it every time).
- as a sandbox to learn the language (as it supports most common features except those which will make implementation not "nano")
- as utility script-running tool, even to create CGI-scripts for web-server

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

| Terminal | Graphics | Mouse | Other |
| - | - | - | - |
| wezterm | OK | OK | Buggy |
| mlterm | OK | | Output flickers when updated. |
| xterm | Sixels work, but no True color | OK | |
| Black Box | OK | ? | On HiDPI system the images are upscaled, and the terminal and the flatpak system use too much CPU time when idling. |
| GNOME Terminal | True color works, but sixel support is not enabled. | | |

![Plotting from nanoclj](https://user-images.githubusercontent.com/6755525/262003070-b5eac109-f1cc-4071-ad7b-a1e5d107a1d9.jpeg "Plotting from nanoclj")
*The plot function returns an image which can then be saved with Image/save or modified using other functions in the Image namespace.*

## Differences to Clojure:

- Characters are 32 bit and strings and char arrays are UTF-8 (length is O(n))
- Strings are compared by their UTF-8 representation
- By default 32 bit integers are used, since 64 bit integers don't fit in the NaN boxing.
- (cons 1 2) is allowed
- List length has complexity of O(n)
- Vectors support O(1) append, but update is O(n)
- Macros and namespace definitions use TinyScheme syntax
- Tail-call optimization
- sort is not stable and cannot throw on type mismatch
- License is BSD 2-Clause License instead of EPL
- No type inheritance
- No 32 bit floating point numbers or small integers
- (identical 'a 'a) ; => true
- Primitives such as doubles and 32 bit integers are passed by value, and are in effect, interned
- Strings are not interned (identical? "abc" "abc") ;=> false
- Ratios use smallest suitable integer type for numerator and denominator
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

- linenoise (included)
- stb_image, stb_image_resize, stb_image_write (included)
- cairo
- libsixel
- utf8proc

## Missing functionality

- Custom types
- Transducers
- Exception handling
- Regular expressions
- Refs, Agents, Atoms, Validators
- Queues
- Arrays
- Tagged Literals (e.g. #uuid and #inst)
- Reader Conditionals
- Namespace qualifiers for keywords
- Exotic numeric literals (e.g. 2r0, 3N, 0.1M)
- Persistent data structures
- Transient data structures
- BigInts and BigDecimals
- Unchecked operations
- Autopromoting operations
- StructMaps
- Multithreading and transactions
- Threading macros (->, -->, some-> and some->>)
- Classes (class, class?, cast)
- `*print-length*`, `*print-level*`
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
  - `*clojure-version*`
  - update, update-in, merge, get-in, disj
  - name
  - lazy-cat, realized?
  - nthrest, nthnext, nfirst
  - parse-long, parse-double, parse-uuid
  - distinct?
  - not-every?, not-any?
  - map-indexed, mapcat
  - partition-by
  - remove
  - merge-with
  - juxt
  - cycle
  - rseq
  - find
  - flatten
  - zipmap
  - memoize
  - group-by
  - condp
  - doto
  - when-not, when-let
  - if-not
  - if-let
  - realized?
  - defn-
  - reduced, reduced?
  - with-local-vars, var-set, find-var, declare
  - binding, 
- clojure.string
  - upper-case
  - clojure.string/capitalize
  - clojure.string/split
  - clojure.string/replace
  - trim, trimr, trim-newline
- clojure.set
- clojure.data.csv
- ...and lots more...
