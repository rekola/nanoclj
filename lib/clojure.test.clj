(def-macro (is arg)
  `(let ((r ,arg))
     (when-not r
       (println "FAIL")
       (println "expected: " ',arg)
       (println "  actual: " r))
     (-is r)))
