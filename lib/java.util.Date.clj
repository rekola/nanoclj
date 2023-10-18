(in-ns 'java.util.Date)

(defn now
  "Returns the current time in milliseconds"
  [] (System/currentTimeMillis))

(defn getTime
  "Returns the time in milliseconds"
  [t] (long t))
