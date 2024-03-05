(ns java.io.Reader
  "A reader"
  (:gen-class))

(defn close
  "Closes the Reader"
  [rdr] (-close rdr))

(defn read
  "Read a single character"
  [rdr] (-read rdr))

(defn readLine
  [this] (let [c (.read this)]
           (if (= c -1)
             nil
             (loop [c c acc ""]
               (if (or (= c -1) (= c 10))
                 acc
                 (recur (.read this) (conj acc (clojure.core/char c))))))))
