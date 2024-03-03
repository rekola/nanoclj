(ns java.lang.Number
  "Superclass for all numeric types"
  (:gen-class :extends java.lang.Object))

(defn byteValue
  "Converts to byte"
  [x] (clojure.core/byte x))

(defn doubleValue
  "Converts to double"
  [x] (clojure.core/double x))

(defn intValue
  "Converts to int"
  [x] (clojure.core/int x))

(defn longValue
  "Converts to long"
  [x] (clojure.core/long x))

(defn shortValue
  "Converts to short"
  [x] (clojure.core/short x))
