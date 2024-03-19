(ns java.lang.Integer
  "A 32-bit integer"
  (:gen-class)
  (:refer-clojure :only (defn read-string number?)))

(def TYPE 4)
(def MAX_VALUE 2147483647)
(def MIN_VALUE -2147483648)

(defn parseInt
  "Parses a string to get an Integer"
  [str] (let [n (read-string str)] (if (number? n) n (throw (new NumberFormatException "Invalid number format")))))
