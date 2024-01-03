(in-ns 'java.lang.String)

(defn toString
  "Returns itself"
  [s] s)

(defn getBytes
  "Returns the string as a byte array"
  [s] (char-array s))

(defn toLowerCase
  "Returns lower case version of the string"
  [s] (clojure.string/lower-case s))

(defn toUpperCase
  "Returns upper case version of the string"
  [s] (clojure.string/upper-case s))

(defn isEmpty
  "Returns true if the string is empty"
  [s] (empty? s))

(defn length
  "Returns the number of codepoints"
  [s] (count s))

(defn startsWith
  ([s substr] (cond (empty? substr) true
                    (= (first s) (first substr)) (.startsWith (rest s) (rest substr))
                    :else false
                    )))
