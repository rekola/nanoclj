(ns clojure.instant)
(import java.util.Date)

(defn read-instant-date
  "Parses a date"
  [cs] (java.util.Date cs))
