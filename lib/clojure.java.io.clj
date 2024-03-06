(ns clojure.java.io
  "Java IO related operations")

(def reader java.io.Reader)
(def writer java.io.Writer)

(def input-stream java.io.InputStream)
(def output-stream java.io.OutputStream)

(defn file
  "Returns a java.io.File"
  ([arg] (java.io.File arg))
  ([parent child] (clojure.string/join (System/getProperty "file.separator") [ parent child ])))

(def as-file java.io.File)
(def as-url java.net.URL)
