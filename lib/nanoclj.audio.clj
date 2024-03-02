(ns nanoclj.audio
  "Audio processing operations")

(defn load
  "Loads an audio file"
  [fn] (Audio/load fn))
