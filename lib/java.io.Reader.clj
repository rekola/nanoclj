(ns java.io.Reader
  "A reader"
  (:gen-class)
  (:refer-clojure :only (defn)))

(def close
  "Closes a Reader"
  clojure.core/-close)

(def read
  "Reads a single character"
  clojure.core/-read)

(defn readLine
  [this] (let [c (.read this)]
           (if (= c -1)
             nil
             (loop [c c acc ""]
               (if (or (= c -1) (= c 10))
                 acc
                 (recur (.read this) (clojure.core/conj acc (clojure.core/char c))))))))
