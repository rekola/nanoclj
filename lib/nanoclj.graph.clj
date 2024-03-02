(ns nanoclj.graph
  "Graph processing operations")

(defn load
  "Loads a graph file"
  [fn] (Graph/load fn))
