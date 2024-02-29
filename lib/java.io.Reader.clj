(ns java.io.Reader)

(defn close
  "Closes the Reader"
  [rdr] (-close rdr))

(defn read
  "Read a single character"
  [rdr] (-read rdr))
