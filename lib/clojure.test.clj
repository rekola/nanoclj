(ns clojure.test
  "Test framework")

(def-macro (is arg)
  `(let ((r ~arg))
     (when-not r
       (println "FAIL in (" *file* ")")
       (println "expected: " (pr-str '~arg))
       (println "  actual: " r))
     (clojure.core/-is r)))
