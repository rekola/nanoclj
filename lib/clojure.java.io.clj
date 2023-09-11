(in-ns 'clojure.java.io)

(def reader java.io.Reader)
(def writer java.io.Writer)

(def file
  "Returns a java.io.File"
  (fn
    ([arg] (java.io.File arg))
    ([parent child] (clojure.string/join (System/getProperty "file.separator") [ parent child ]))))

(def as-file java.io.File)
