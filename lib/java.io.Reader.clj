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
  [this] (let [c0 (.read this)]
           (if (= c0 -1) nil
               (loop [c c0 acc ""]
                 (if (or (= c -1) (= c \newline)) acc (recur (.read this) (conj acc c)))))))
