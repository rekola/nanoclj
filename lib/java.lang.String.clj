(ns java.lang.String
  "A class that represents a UTF8 string"
  (:gen-class)
  (:refer-clojure :only (defn empty? count < = bit-or)))

(def UTF8PROC_COMPOSE 4)
(def UTF8PROC_IGNORE 32)
(def UTF8PROC_STRIPCC 512)
(def UTF8PROC_CASEFOLD 1024)

(defn toString
  "Returns itself"
  [s] s)

(defn getBytes
  "Returns the string as a byte array"
  [s] (clojure.core/char-array s))

(defn toLowerCase
  "Returns lower case version of the string"
  [s] (clojure.core/-utf8map s (bit-or UTF8PROC_IGNORE UTF8PROC_STRIPCC UTF8PROC_CASEFOLD UTF8PROC_COMPOSE)))

(defn toUpperCase
  "Returns upper case version of the string"
  [s] (clojure.core/maps clojure.core/-toupper s))

(defn isEmpty
  "Returns true if the string is empty"
  [s] (empty? s))

(defn length
  "Returns the number of codepoints"
  [s] (count s))

(defn startsWith
  ([s substr] (let [n1 (count s)
                    n2 (count substr)
                    ]
                (if (< n1 n2)
                  false
                  (= (clojure.core/subs s 0 n2) substr)))))
