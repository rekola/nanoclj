(ns clojure.set)

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
                                    (if (contains? B (first A))
                                      (recur (conj r (first A)) (next A))
                                      (recur r (next A)))
                                    r)))
  ([A B & more] (reduce intersection (intersection A B) more)))

(defn difference
  "Returns a set with elements in A that do not belong to the other sets"
  ([A] A)
  ([A B] (loop [r #{ } A (seq A)] (if A
                                    (if (contains? B (first A))
                                      (recur r (next A))
                                      (recur (conj r (first A)) (next A)))
                                    r)))
  ([A B & more] (reduce difference (difference A B) more)))

(defn superset?
  "Returns true if A is a superset of B"
  ([A B] (loop [B (seq B)] (if B
                             (if (contains? A (first B))
                               (recur (next B))
                               false)
                             true))))

(defn select
  [pred A] (loop [r #{ } A (seq A)] (if A
                                      (if (pred (first A))
                                        (recur (conj r (first A)) (next A))
                                        (recur r (next A)))
                                      r)))
