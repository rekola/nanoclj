; Initialization file for nanoclj

(load-file "bootstrap.clj")

(import java.lang.System)
(import java.lang.Thread)
(import java.lang.Math)
(import java.lang.Throwable)
(import java.security.SecureRandom)
(import clojure.lang.Keyword)
(import clojure.lang.BigInt)
(import clojure.lang.MapEntry)
(import clojure.lang.LazySeq)
(import clojure.lang.APersistentSet)
(import clojure.lang.APersistentMap)
(import clojure.lang.Ratio)
(import clojure.lang.Var)
(import clojure.lang.PersistentQueue)
(import clojure.lang.Delay)
(import java.util.regex.Pattern)
(import nanoclj.lang.Gradient)
(import nanoclj.lang.EmptyList)

; Most of the classes have already been defined, so :reload flag is needed to trigger the load
(require 'java.lang.Object :reload :import)
(require 'java.lang.Class :reload :import)
(require 'java.lang.Integer :reload :import)
(require 'java.lang.Float :reload :import)
(require 'java.lang.Double :reload :import)
(require 'java.lang.Boolean :reload :import)
(require 'java.lang.Byte :reload :import)
(require 'java.lang.Short :reload :import)
(require 'java.lang.Long :reload :import)
(require 'java.lang.String :reload :import)
(require 'java.util.Date :reload :import)
(require 'java.util.UUID :reload :import)
(require 'clojure.lang.PersistentVector :reload :import)
(require 'nanoclj.lang.Codepoint :reload :import)
(require 'nanoclj.lang.Tensor :reload :import)
(require 'nanoclj.lang.Image :reload :import)

(load-file "clojure.core.clj")
(load-file "clojure.repl.clj")
