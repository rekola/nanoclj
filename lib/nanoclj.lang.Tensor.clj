(ns nanoclj.lang.Tensor
  "A class that represents tensors"
  (:gen-class)
  (:refer-clojure :only (defn)))

(def tensor nanoclj.lang.Tensor)

(defn clone
  "Creates a clone of this"
  [this] (clojure.core/aclone this))
