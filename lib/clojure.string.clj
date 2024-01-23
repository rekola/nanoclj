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
  ([s value] (index-of s value 0))
  ([s value from-index] (loop [idx from-index]
                          (cond (>= idx (- (count s) (count value))) nil
                                (= (subs s idx (+ idx (count value))) value) idx
                                :else (recur (inc idx))))))

(defn last-index-of
  "Returns the last index of value in s"
  ([s value] (last-index-of s value 0))
  ([s value from-index] (loop [idx (- (count s) (count value))]
                          (cond (or (< idx 0) (< idx from-index)) nil
                                (= (subs s idx (+ idx (count value))) value) idx
                                :else (recur (dec idx))))))

(defn split
  "Splits string to tokens using a regex"
  ([s re] (split s re 0))
  ([s re limit] (loop [acc [] s s]
                  (if (and (> limit 0) (= (count acc) (dec limit)))
                    (conj acc s)
                    (let [indices (re-find-index re s)]
                      (if indices
                        (recur (conj acc (subs s 0 (indices 0))) (subs s (indices 1)))
                        (if (and (= limit 0) (empty? s)) acc (conj acc s))))))))

(defn replace-first
  "Replaces the first occurence of match with replacement"
  [s match replacement] (let [indices (re-find-index match s)]
                          (if indices
                            (str (subs s 0 (indices 0)) replacement (subs s (indices 1)))
                            s)))

(defn replace
  "Replaces all occurences of match with replacement"
  [s match replacement] (let [indices (re-find-index match s)]
                          (if indices
                            (str (subs s 0 (indices 0)) replacement (replace (subs s (indices 1)) match replacement))
                            s)))

(defn split-lines
  "Split input to lines"
  [s] (split s #"\r?\n"))

(defn escape
  "Escapes characters"
  [s cmap] (loop [ acc "" s (seq s) ]
             (if s
               (let [f (first s)
                     rep (get cmap f) ]
                 (if rep
                   (recur (str acc rep) (next s))
                   (recur (conj acc f) (next s))))
               acc)))

(defn re-quote-replacement
  "Makes replacement a literal strings that doesn't capture any references in replace"
  [replacement] (escape replacement { \\ "\\\\" \$ "\\$" }))
