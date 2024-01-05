(def-macro (is arg)
  `(let ((r ,arg))
     (when-not r
       (println "FAIL in (" *file* ")")
       (println "expected: " ',arg)
       (println "  actual: " r))
     (-is r)))
