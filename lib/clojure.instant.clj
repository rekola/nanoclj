(ns clojure.instant
  "Time related operations")
(import java.util.Date)

(defn read-instant-date
  "Parses a date"
  [cs] (java.util.Date cs))
