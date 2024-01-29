(ns java.lang.Long)

(def MAX_VALUE 9223372036854775807)
(def MIN_VALUE -9223372036854775808)

(def parseLong
  "Parses a string to get a Long"
  (fn [str] (let [n (read-string str)] (if (number? n) n (throw (new NumberFormatException "Invalid number format"))))))
