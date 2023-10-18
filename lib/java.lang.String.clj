(in-ns 'java.lang.String)

(defn toString
  "Returns itself"
  [s] s)

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
