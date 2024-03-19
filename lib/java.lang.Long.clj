(ns java.lang.Long
  "A 64-bit integer"
  (:gen-class)
  (:refer-clojure :only (defn read-string number?)))

(def MAX_VALUE 9223372036854775807)
(def MIN_VALUE -9223372036854775808)

(defn parseLong
  "Parses a string to get a Long"
  [str] (let [n (read-string str)] (if (number? n) n (throw (new NumberFormatException "Invalid number format")))))
