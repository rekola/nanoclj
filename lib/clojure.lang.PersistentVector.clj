(ns clojure.lang.PersistentVector
  "A persistent vector"
  (:gen-class))

(def EMPTY [])

(defn indexOf
  [v o] (loop [v (seq v) idx 0]
          (if v
            (if (= (first v) o) idx (recur (next v) (inc idx)))
            nil)))

