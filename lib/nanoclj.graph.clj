(in-ns 'nanoclj.graph)

(defn load
  "Loads a graph file"
  [fn] (Graph/load fn))

(defn update-layout
  "Updated graph layout"
  [g] (Graph/updateLayout g))
