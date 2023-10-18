(in-ns 'java.lang.Long)

(def MAX_VALUE 9223372036854775807)
(def MIN_VALUE -9223372036854775808)

(def parseLong (fn [str] (let [n (read-string str)] (if (number? n) n nil))))
