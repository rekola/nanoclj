(in-ns 'java.lang.Integer)

(def MAX_VALUE 2147483647)
(def MIN_VALUE -2147483648)

(def parseInt
  "Parses a string to get an Int"
  (fn [str] (let [n (read-string str)] (if (number? n) n nil))))
