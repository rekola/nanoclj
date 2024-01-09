(in-ns 'java.lang.String)

(def UTF8PROC_COMPOSE 4)
(def UTF8PROC_IGNORE 32)
(def UTF8PROC_STRIPCC 512)
(def UTF8PROC_CASEFOLD 1024)

(defn toString
  "Returns itself"
  [s] s)

(defn getBytes
  "Returns the string as a byte array"
  [s] (char-array s))

(defn toLowerCase
  "Returns lower case version of the string"
  [s] (utf8map s (bit-or UTF8PROC_IGNORE UTF8PROC_STRIPCC UTF8PROC_CASEFOLD UTF8PROC_COMPOSE)))

(defn toUpperCase
  "Returns upper case version of the string"
  [s] (maps -toupper s))

(defn isEmpty
  "Returns true if the string is empty"
  [s] (empty? s))

(defn length
  "Returns the number of codepoints"
  [s] (count s))

(defn startsWith
  ([s substr] (loop [s s substr substr]
                (cond (empty? substr) true
                      (empty? s) false
                      (= (first s) (first substr)) (recur (rest s) (rest substr))
                      :else false))))
