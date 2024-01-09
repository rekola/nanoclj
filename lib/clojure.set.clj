(in-ns 'clojure.set)

(defn union
  "Returns the union of the given sets"
  ([] #{})
  ([A] A)
  ([A B] (loop [ r A B (seq B) ] (if B (recur (conj r (first B)) (next B)) r)))
  ([A B & more ] (reduce union (union A B) more)))

(defn intersection
  "Returns the intersection of the given sets"
  ([A] A)
  ([A B] (loop [r #{ } A (seq A)] (if A
                                    (if (get B (first A))
                                      (recur (conj r (first A)) (next A))
                                      (recur r (next A)))
                                    r)))
  ([A B & more] (reduce intersection (intersection A B) more)))

