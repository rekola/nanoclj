(in-ns 'Integer)

(def parseInt (fn [str] (let [n (read-string str)] (if (number? n) n false))))
(def MAX_VALUE 2147483647)
(def MIN_VALUE -2147483648)
