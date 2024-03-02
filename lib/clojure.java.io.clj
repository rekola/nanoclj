(ns clojure.java.io
  "Java IO related operations")
(import java.io.Reader)
(import java.io.Writer)
(import java.io.InputStream)
(import java.io.OutputStream)
(import java.io.File)
(import java.net.URL)

(def reader java.io.Reader)
(def writer java.io.Writer)

(def input-stream java.io.InputStream)
(def output-stream java.io.OutputStream)

(def file
  "Returns a java.io.File"
  (fn
    ([arg] (java.io.File arg))
    ([parent child] (clojure.string/join (System/getProperty "file.separator") [ parent child ]))))

(def as-file java.io.File)
(def as-url java.net.URL)
