(ns java.io.Writer
  "A writer"
  (:gen-class))

(defn close
  "Closes the Writer"
  [w] (-close w))
