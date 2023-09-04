(in-ns 'clojure.string)

(def UTF8PROC_COMPOSE 4)
(def UTF8PROC_IGNORE 32)
(def UTF8PROC_STRIPCC 512)
(def UTF8PROC_CASEFOLD 1024)

(defn blank?
  "Returns true if the argument string is blank"
  [s] (if (empty? s)
        true
        (if (case (first s)
              \space true
              \newline true
              \return true
              \tab true
              \formfeed true
              \backspace true
              false
              )
          (recur (rest s))
          false)))

(defn lower-case [s] (utf8map s (bit-or UTF8PROC_IGNORE UTF8PROC_STRIPCC UTF8PROC_CASEFOLD UTF8PROC_COMPOSE)))

(def join (fn ([coll]           (apply str coll))
              ([separator coll] (if (empty? coll) "" (if (empty? (rest coll)) (first coll) (str (first coll) separator (join separator (rest coll))))))))

(defn reverse [s] (apply str (rseq s)))

(defn starts-with?
  "Returns true if s starts with substr"
  [ s substr ] (cond (empty? substr) true
                     (= (first s) (first substr)) (starts-with? (rest s) (rest substr))
                     :else false
                     ))

(defn includes?
  "Returns true if s includes substr"
  [ s substr ] (if (empty? s)
                 (empty? substr)
                 (if (starts-with? s substr)
                   true
                   (recur (rest s) substr))))

(defn triml
  [s] (apply str (drop-while (fn [c] (case c
                                       \space true
                                       \newline true
                                       \return true
                                       \tab true
                                       \formfeed true
                                       \backspace true
                                       false
                                       )) s)))

