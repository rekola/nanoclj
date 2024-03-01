(ns java.io.Reader
  "A reader"
  (:gen-class))

(defn close
  "Closes the Reader"
  [rdr] (-close rdr))

(defn read
  "Read a single character"
  [rdr] (-read rdr))
