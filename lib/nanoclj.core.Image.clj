(in-ns 'nanoclj.core.Image)

(defn getWidth
  "Returns the width of the image"
  [i] (i :width))

(defn getHeight
  "Returns the height of the image"
  [i] (i :height))
