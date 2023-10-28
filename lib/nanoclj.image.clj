(in-ns 'nanoclj.image)

(defn gaussian-blur
  "Applies gaussian blur to image"
  [radius image]
  (.transpose
   (.horizontalGaussianBlur radius
                            (.transpose
                             (.horizontalGaussianBlur radius image)))))

(defn load
  "Loads an image"
  [fn] (Image/load fn))

(defn save
  "Saves an image"
  [image fn] (.save image fn))

(defn transpose
  "Transposes an image"
  [image] (.transpose image))
