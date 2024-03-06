(ns clojure.instant
  "Time related operations")

(defn read-instant-date
  "Parses a date"
  [cs] (java.util.Date cs))
