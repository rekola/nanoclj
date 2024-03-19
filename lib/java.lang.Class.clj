(ns java.lang.Class
  "A class that represents classes"
  (:gen-class)
  (:refer-clojure :only (defn str)))

(defn getName
  "Returns the class name"
  [c] (str c))
