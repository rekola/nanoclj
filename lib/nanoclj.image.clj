(ns nanoclj.image
  "Image processing operations")

(defn gaussian-blur
  "Applies gaussian blur to image"
  [i radius]
  (.transpose
   (.horizontalGaussianBlur (.transpose
                             (.horizontalGaussianBlur (image i) radius)) radius)))

(defn load
  "Loads an image"
  [fn] (Image/load fn))

(defn save
  "Saves an image"
  [i fn] (.save (image i) fn))

(defn transpose
  "Transposes an image"
  [i] (.transpose (image i)))
