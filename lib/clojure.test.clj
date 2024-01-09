(in-ns 'clojure.test)

(def-macro (is arg)
  `(let ((r ,arg))
     (when-not r
       (println "FAIL in (" *file* ")")
       (println "expected: " (pr-str ',arg))
       (println "  actual: " r))
     (-is r)))
