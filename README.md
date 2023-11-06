# nanoclj

[![CI](https://github.com/rekola/nanoclj/workflows/Ubuntu-CI/badge.svg)]()
[![CI](https://github.com/rekola/nanoclj/workflows/VS17-CI/badge.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

## A Tiny Clojure Interpreter

This is a small Clojure language implementation written in C
language. It is based on TinyScheme which was based on MiniScheme.

## Features

- Terminal graphics (*Kitty* or *Sixel* protocols)
- Image, audio, Shapefile, XML, CSV and GraphML loading
- Simple image operations (blur, transpose etc.) and 2D canvas, with HiDPI support
- REPL output is colored by type
- Callback Writer for printing into a GUI instead of stdout
- Class and namespace names try to imitate Java and Clojure names when possible (e.g. `(type 1) ;=> java.lang.Long`)

### 2D Graphics

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
the GUI callbacks for output Writer. In terminal program Kitty or
Sixel protocol is used to print the results of a function call when
the function returns a canvas or image. The terminal must, of course,
need to support Kitty or Sixel graphics and support must be
enabled. The following terminals have been tested:

| Terminal | Graphics | Mouse | Notes |
| - | - | - | - |
| foot | OK | OK | Wayland only |
| kitty | OK | ? | True color images, but window resizing has bugs (as of 0.26.5) |
| wezterm | OK | OK | Buggy (as of 20230712) |
| mlterm | OK | ? | linenoise output flickers when updated |
| Konsole | OK | ? | True color images, but on HiDPI system images are upscaled |
| contour | Inline image layout doesn't work | ? | |
| xterm | No true color	| OK | Sixel support must be enabled in `.Xresources` |
| Black Box | Inline image layout doesn't work | ? | On HiDPI system the images are upscaled, and the terminal and the flatpak system use too much CPU time when idling. |
| Alacritty | None | ? | |
| GNOME Terminal | None | ? | Sixel support is not enabled by default |
| mintty | ? | ? | Not tested yet. |

![Plotting from nanoclj](https://user-images.githubusercontent.com/6755525/277504459-737b498e-005b-49ad-92b2-0917a1a10b7e.jpg "Plotting from nanoclj")
*The plot function returns a canvas, which can be converted to an image, and then saved with Image/save or modified using other functions in the Image namespace.*

As well as printing images in block mode like the plot function does, they can also be printed in inline mode which is shown in the following example:

![Inline images](https://user-images.githubusercontent.com/6755525/277514315-6b5f26a0-a1ab-4f66-95b7-4c976f288ff3.jpg "Inline images")

## Differences to Clojure:

- Characters are 32 bit and strings and char arrays are UTF-8 (count is O(n))
- Strings are sequences, and they are compared and sorted as such.
- List count has complexity of O(n)
- Vectors support O(1) append, but update is O(n)
- Macros use the TinyScheme syntax
- Tail-call optimization
- sort is not stable and doesn't throw on mismatching types
- License is BSD 2-Clause License instead of EPL
- No 32-bit floating point numbers or small integers
- Symbols are interned and cannot have metadata
- Primitives such as doubles and 32 bit integers are passed by value, and are in effect, interned
- Strings are not interned: (identical? "abc" "abc") ;=> false
- `rationalize` is exact and Ratios use the smallest suitable integer type for numerator and denominator
- No chunked or buffered lazy sequences

## Dependencies

- linenoise (included, utf8 support added)
- stb_image, stb_image_resize, stb_image_write (included)
- dr_wav (included)
- ggml (included, experimental)
- zlib
- libxml2
- pcre2
- cairo
- libsixel
- utf8proc
- shapelib

## Building

### Ubuntu

```
sudo apt install libutf8proc-dev libsixel-dev libpcre2-dev libcairo2-dev libshp-dev libcurl4-dev libxml2-dev libz-dev
mkdir build
cd build
cmake ..
make
```

### Windows

Windows support is in progress.

## Missing functionality

- Custom types
- Transducers
- Refs, Agents, Atoms, Validators
- Arrays, StructMaps
- Reader Conditionals
- BigInts, BigDecimals and Exotic numeric literals (e.g. 3N, 0.1M, hexadecimal floats)
- Persistent data structures, Transient data structures
- Unchecked operations
- Autopromoting operations
- Interfaces, Records, Protocols and Multi-methods
- Locals cleaning
- Multithreading, transactions and STM
- monitor-enter, monitor-exit, and locking
- Metadata reader macro, Threading macros (->, -->, some-> and some->>)
- Homogenous vectors (vector-of)
- Pre- and post-conditions for functions
- Readers and Writers do not accept options such as :encoding or :append
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
  - with-local-vars, var-set, find-var, alter-var-root, declare, binding, with-bindings
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
  - thread, thread-call
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
- clojure.pprint
  - print-table
- clojure.set
- clojure.data
- clojure.walk
- clojure.zip
- clojure.spec
- ...and lots more...
