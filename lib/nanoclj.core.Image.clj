(ns nanoclj.core.Image)

(def TYPE_CUSTOM 0)
(def TYPE_BYTE_GRAY 1)
(def TYPE_INT_RGB 5)
(def TYPE_INT_ARGB_PRE 6)

; Unimplemented types: TYPE_INT_BGR, TYPE_USHORT_565_RGB, TYPE_USHORT_555_RGB, TYPE_BYTE_BINARY, TYPE_BYTE_INDEXED, TYPE_USHORT_GRAY, TYPE_4BYTE_ABGR, TYPE_INT_ARGB, TYPE_4BYTE_ABGR_PRE, TYPE_3BYTE_BGR

(defn getWidth
  "Returns the width of the image"
  [i] (i :width))

(defn getHeight
  "Returns the height of the image"
  [i] (i :height))

(defn getType
  "Returns the type of the image"
  [i] (i :type))
