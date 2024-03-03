(ns java.lang.Double
  "A 64-bit floating point number"
  (:gen-class))

(def TYPE 6)
(def BYTES 8)
(def NaN ##NaN)
(def NEGATIVE_INFINITY ##-Inf)
(def POSITIVE_INFINITY ##Inf)
(def SIZE 64)

(defn parseDouble
  "Parses a string to get a Double"
  [str] (double str))

(defn isInfinite
  "Returns true if argument is infinite"
  [d] (or (== d ##Inf) (== d ##-Inf)))

(defn isFinite
  "Returns true if argument is finite"
  [d] (and (not= d ##Inf) (not= d ##-Inf) (== d d)))

(defn isNaN
  "Returns true if argument is a NaN"
  [d] (not= d d))

(defn equals
  "Returns true if this is equal to the other"
  [this other] (and (clojure.core/isa? java.lang.Double other)
                    (= (doubleToLongBits this) (doubleToLongBits other))))

(defn sum
  "Returns the sum of d1 and d2"
  [d1 d2] (+ d1 d2))

(defn valueOf
  "Returns the double value itself"
  [v] (double v))
