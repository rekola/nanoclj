(in-ns 'clojure.string)

(defn blank?
  "Returns true if the argument string is blank"
  [s] (cond (empty? s) true
            (.isWhitespace (first s)) (recur (rest s))
            :else false))

(defn upper-case
  "Converts all letters to upper-case"
  [s] (.toUpperCase s))

(defn lower-case
  "Converts all letters to lower-case"
  [s] (.toLowerCase s))

(defn capitalize
  "Capitalizes the first letter and puts the rest in lower-case"
  [s] (str (.toTitleCase (first s)) (lower-case (rest s))))

(def join (fn ([coll]           (apply str coll))
              ([separator coll] (if (empty? coll) "" (if (empty? (rest coll)) (first coll) (str (first coll) separator (join separator (rest coll))))))))

(defn reverse [s] (apply str (rseq s)))

(defn starts-with?
  "Returns true if s starts with substr"
  [ s substr ] (.startsWith s substr))

(defn includes?
  "Returns true if s includes substr"
  [ s substr ] (if (empty? s)
                 (empty? substr)
                 (if (starts-with? s substr)
                   true
                   (recur (rest s) substr))))

(defn triml
  "Trims whitespace from the left side of a string"
  [s] (apply str (drop-while #( .isWhitespace % ) s)))

(defn trimr
  "Trims whitespace from the right side of a string"
  [s] (apply str (reverse (drop-while #( .isWhitespace % ) (rseq s)))))

(defn trim
  "Trims whitespace from both sides of a string"
  [s] (triml (trimr s)))

(defn trim-newline
  "Trims trailing newlines and carriage returns from a string"
  [s] (apply str (reverse (drop-while (fn [c] (case c
                                       \newline true
                                       \return true
                                       false
                                       )) (rseq s)))))

(defn index-of
  "Returns the index of substring value in s, or nil if not found"
  [s value] (loop [s (seq s) idx 0]
              (cond (nil? s) nil
                    (.startsWith s value) idx
                    :else (recur (next s) (inc idx)))))
