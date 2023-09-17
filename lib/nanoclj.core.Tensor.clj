(in-ns 'nanoclj.core.Tensor)

(def tensor nanoclj.core.Tensor)

(defn ->tensor
  [coll] (let [size (count coll)
               t (tensor size)]
           (loop [coll coll i 0]
             (if (empty? coll) t
                 (do (tensor-set! t i (first coll))
                     (recur (rest coll) (inc i)))))))
