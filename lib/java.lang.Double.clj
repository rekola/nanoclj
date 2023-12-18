(in-ns 'java.lang.Double)

(def TYPE 5)
(def BYTES 8)
(def NaN ##NaN)
(def NEGATIVE_INFINITY ##-Inf)
(def POSITIVE_INFINITY ##Inf)
(def SIZE 64)

(def parseDouble
  "Parses a string to get a Double"
  (fn [str] (double str)))

(def isInfinite
  "Returns true if argument is infinite"
  (fn [d] (or (== d ##Inf) (== d ##-Inf))))

(def isNaN
  "Returns true if argument is a NaN"
  (fn [d] (== d ##NaN)))




