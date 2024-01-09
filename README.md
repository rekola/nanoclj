# nanoclj

[![CI](https://github.com/rekola/nanoclj/workflows/Ubuntu-CI/badge.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

## A Tiny Clojure Interpreter

This is a small Clojure language implementation written in C
language. It is based on TinyScheme which was based on MiniScheme. The
project is in the early stages yet, and much functionality is still
missing. The project has two main goals:

1. Provide data visualization capabilities for the REPL in a terminal or GUI.
2. Provide an embedded Clojure interpreter for C++ applications as an alternative for Lua and other scripting languages.

## Features

- Terminal graphics (*Kitty* or *Sixel* protocols) with HiDPI support
- Image, audio, Shapefile, XML, CSV and GraphML loading
- Simple image operations (blur, transpose etc.) and 2D canvas
- REPL output is colored by type
- Callback Writer for printing into a GUI instead of stdout
- Class and namespace names try to imitate Java and Clojure names when possible (e.g. `(type 1) ;=> java.lang.Long`)
- BigInts and Ratios
- Tensors are used for representing most data structures. For example, a vector is a 1D tensor of doubles (with NaN-packing for other data types).
- Test framework

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

![Plotting from nanoclj](https://github.com/rekola/nanoclj/assets/6755525/194a6914-f87a-446f-8bae-53b12c75b327 "Plotting from nanoclj")
*The plot function returns a canvas, which can be converted to an image, and then saved with Image/save or modified using other functions in the Image namespace.*

As well as printing images in block mode like the plot function does, they can also be printed in inline mode which is shown in the following example:

![Inline gradients](https://github.com/rekola/nanoclj/assets/6755525/170b337f-d9b1-4c6c-9d5d-7e80e1e9fb37 "Inline gradients")

## Differences to Clojure:

- Characters are 32 bit, strings are UTF-8 (count is O(n)) and char-array creates a byte array
- Strings are sequences, and they are compared and sorted as such.
- List count has complexity of O(n)
- Vectors support O(1) append, but update is O(n)
- Macros use the TinyScheme syntax
- Tail-call optimization
- sort is not stable and doesn't support custom compare function
- License is BSD 3-Clause License instead of EPL
- Symbols are interned but are temporarily boxed when metadata is added
- Primitives such as doubles and small integers are passed by value, and are in effect, interned
- Strings are not interned: `(identical? "abc" "abc") ;=> false`
- `rationalize` returns exact result for doubles: `(rationalize 0.1) ;=> 3602879701896397/36028797018963968`
- No chunked or buffered lazy sequences
- No homogenous vectors (They are not necessary since most data types are unboxed by default)
- Data structures are only partially persistent, and while vectors, maps and sets allow fast reading and insertion, deletion and modification is slow.
- Vectors, maps, sets and queues cannot have metadata
- Namespaces can only contain Vars, not Classes: `(resolve 'Math) ;=> #'java.lang.Math
- Arrays are compared by value, can only contain primitive values (including Objects) and multidimensional arrays cannot be ragged
- No type hints
- Unbound Vars cannot be created

## Dependencies

- linenoise (included, extra functionality such as UTF8 support and brace highlighting added)
- stb_image, stb_image_resize, stb_image_write (included)
- dr_wav (included)
- zlib
- libxml2
- pcre2
- cairo
- libsixel
- utf8proc
- shapelib

## Building

C11 support is required for building nanoclj, and currently it has only been tested with gcc. At the moment, it's unlikely that nanoclj would work on a 32 bit system.

### Ubuntu

```
sudo apt install libutf8proc-dev libsixel-dev libpcre2-dev libcairo2-dev libshp-dev libcurl4-gnutls-dev libxml2-dev libz-dev
mkdir build
cd build
cmake ..
make
sudo make install
```

### Arch Linux PKGBUILD

```
curl https://raw.githubusercontent.com/rekola/nanoclj/main/PKGBUILD -O
makepkg -si
```

### Windows

Windows support is in progress.

## Missing functionality

- Interfaces, Records, StructMaps, Protocols and Multi-methods
- Transient data structures and dynamic variables
- Transducers
- Refs, Agents, Atoms, Validators
- Reader Conditionals
- BigDecimals, 32-bit floats and hexadecimal floating point literals
- Unchecked operations
- Locals cleaning
- Multithreading, transactions, STM and locking
- Pre- and post-conditions for functions
- Readers and Writers do not accept options such as :encoding or :append
- Associative destructuring
- Map namespace syntax
- `*print-length*`, `*print-level*`, `*clojure-version*`, `*load-tests*`
- Missing core functions and macros
  - doseq, for, dotimes
  - sorted-set-by, sorted-set, sorted-map, sorted-map-by
  - update-in, merge-with
  - doto, ->, -->, some->, some->>
  - map-indexed, mapcat, lazy-cat
  - partition-by
  - memoize
  - condp
  - when-let, letfn, if-let, if-some
  - reduced, reduced?
  - with-local-vars, var-set, find-var, alter-var-root, declare, binding, with-bindings
  - sequence, subseq, rsubseq
  - make-hierarchy, ancestors, supers, bases, underive
  - select-keys
  - bound?
  - random-uuid
  - re-groups, re-matcher, re-seq, re-matches
  - assert-args
  - with-open
  - pop!, conj!, persistent!
  - fnil
  - trampoline
  - unchecked-byte, unchecked-short, unchecked-char, unchecked-int, unchecked-long, unchecked-float, unchecked-double
  - vary-meta, alter-meta!, reset-meta!
  - aset-char, aset-long, amap, areduce, to-array-2d, make-array, long-array, bytes?
  - remove-ns, create-ns, ns-imports, ns-interns, ns-refers, ns-publics, ns-aliases, ns-name, all-ns, ns-unalias, ns-unmap, import, ns
  - find-keyword
- clojure.java.io
  - resource
  - make-parents
- clojure.math
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
