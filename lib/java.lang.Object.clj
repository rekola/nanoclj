(ns java.lang.Object
  "A base object for everything else"
  (:gen-class)
  (:refer-clojure :only (defn)))

(def TYPE 7)

(defn toString
  "Creates a string representation"
  [this] (str this))

(defn equals
  "Returns true if this is equal to the other"
  [this other] (= this other))

(defn clone
  "Creates a clone of this, or copy if the object is immutable"
  [this] this)
