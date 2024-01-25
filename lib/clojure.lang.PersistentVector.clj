(in-ns 'clojure.lang.PersistentVector)

(def EMPTY [])

(defn indexOf
  [v o] (loop [v (seq v) idx 0]
          (if v
            (if (= (first v) o) idx (recur (next v) (inc idx)))
            nil)))

