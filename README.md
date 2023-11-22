# nanoclj

[![CI](https://github.com/rekola/nanoclj/workflows/Ubuntu-CI/badge.svg)]()
[![CI](https://github.com/rekola/nanoclj/workflows/VS17-CI/badge.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

## A Tiny Clojure Interpreter

This is a small Clojure language implementation written in C
language. It is based on TinyScheme which was based on MiniScheme. The
project is in the early stages yet, and much functionality is still
missing. The project has two main goals:

1. Provide data visualization capabilities for the REPL in a terminal or GUI.
2. Provide an embedded Clojure interpreter for C++ applications as an alternative for Lua and other scripting languages.

## Features

- Terminal graphics (*Kitty* or *Sixel* protocols)
- Image, audio, Shapefile, XML, CSV and GraphML loading
- Simple image operations (blur, transpose etc.) and 2D canvas, with HiDPI support
- REPL output is colored by type
- Callback Writer for printing into a GUI instead of stdout
- Class and namespace names try to imitate Java and Clojure names when possible (e.g. `(type 1) ;=> java.lang.Long`)
- BigInts and Ratios
- Tensors are used for representing most data structures. For example, a vector is a 1D tensor of doubles (with NaN-packing for other data types).

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

As well as printing images in block mode like the plot function does, they can also be printed in inline mode which is shown in the following examples:

![Inline images](https://user-images.githubusercontent.com/6755525/277514315-6b5f26a0-a1ab-4f66-95b7-4c976f288ff3.jpg "Inline images")

![Inline gradients](https://user-images.githubusercontent.com/6755525/283999369-87e10299-9ab6-4f49-b5bb-62e653cba226.png "Inline gradients")

## Differences to Clojure:

- Characters are 32 bit and strings and char arrays are UTF-8 (count is O(n))
- Strings are sequences, and they are compared and sorted as such.
- List count has complexity of O(n)
- Vectors support O(1) append, but update is O(n)
- Macros use the TinyScheme syntax
- Tail-call optimization
- sort is not stable and doesn't throw on mismatching types
- License is BSD 2-Clause License instead of EPL
- Symbols are interned and cannot have metadata
- Primitives such as doubles and 32 bit integers are passed by value, and are in effect, interned
- Strings are not interned: `(identical? "abc" "abc") ;=> false`
- `rationalize` returns exact result for doubles: `(rationalize 0.1) ;=> 3602879701896397/36028797018963968`
- No chunked or buffered lazy sequences
- No homogenous vectors (They are not necessary since most data types are unboxed by default)

## Dependencies

- linenoise (included, utf8 support added)
- stb_image, stb_image_resize, stb_image_write (included)
- dr_wav (included)
- zlib
- libxml2
- pcre2
- cairo
- libsixel
- utf8proc
- shapelib
- ffmpeg

## Building

### Ubuntu

```
sudo apt install libutf8proc-dev libsixel-dev libpcre2-dev libcairo2-dev libshp-dev libcurl4-gnutls-dev libxml2-dev libz-dev libavcodec-dev
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
- Reader Conditionals
- BigDecimals, 32-bit floats and hexadecimal floating point literals
- Persistent data structures, Transient data structures, StructMaps
- Unchecked operations
- Interfaces, Records, Protocols and Multi-methods
- Locals cleaning
- Multithreading, transactions and STM
- monitor-enter, monitor-exit, and locking
- Metadata reader macro
- Pre- and post-conditions for functions
- Namespace import and aliases
- Readers and Writers do not accept options such as :encoding or :append
- Associative destructuring
- `*print-length*`, `*print-level*`, `*file*`, `*flush-on-newline*`, `*clojure-version*`, `*load-tests*`, `*print-meta*`
- Missing core functions and macros
  - doseq, for, dotimes
  - bit-and-not
  - sort-by, sorted-set-by, sorted-map, hash-set, hash-map
  - update-in, merge-with
  - cast, num
  - doto, ->, -->, some->, some->>
  - parse-long, parse-double, parse-uuid
  - map-indexed, mapcat, zipmap, lazy-cat
  - partition-by
  - juxt
  - memoize
  - group-by
  - condp
  - when-let, letfn, if-let, if-some
  - reduced, reduced?
  - with-local-vars, var-set, find-var, alter-var-root, declare, binding, with-bindings, ns-name
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
  - int-array, long-array, float-array, double-array, byte-array, short-array, aset-int, aset-double, object-array
  - aget, aset, alength, amap, areduce, aclone, into-array, make-array, to-array-2d, to-array
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
