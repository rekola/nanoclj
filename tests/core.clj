(require '[ clojure.test :as t ])
                                        ; Primitives
(t/is (integer? 1))
(t/is (number? 1.23))

(t/is (instance? Boolean true))
(t/is (instance? Boolean false))
(t/is (instance? Long 1))
(t/is (instance? Codepoint \a))
(t/is (instance? EmptyList '()))
(t/is (instance? Double 0.1))
(t/is (instance? Byte (byte 1)))
(t/is (instance? Short (short 1)))
(t/is (instance? Integer (int 1)))

(t/is (instance? Keyword :abc))
(t/is (instance? Symbol 'abc))
(t/is (= "bb" (name :aa/bb)))
(t/is (= "aa" (namespace 'aa/bb)))

(t/is (instance? Double ##Inf))
(t/is (instance? Double ##-Inf))
(t/is (instance? Double ##NaN))

                                        ; Arithmetics

(t/is (= 6 (+ 1 2 3)))

(t/is (= (abs -100N) 100))

(t/is (= (inc 5) 6))
(t/is (= (dec 6) 5))

                                        ; Bigints

(t/is (instance? BigInt 1N))
(t/is (= (* 1000000000N 1000000000N) 1000000000000000000N))
(t/is (= (* 100000000000000000000 100000000000000000000)
       10000000000000000000000000000000000000000))
(t/is (= (/ 100000000000000000000000000 1000000000000000000000000) 100N))

                                        ; Ratios

(t/is (ratio? 1/2))
(t/is (instance? Long 2/1))
(t/is (instance? BigInt 10000000000000000000/2))

(t/is (= 1/2 (/ 1 2)))
(t/is (= (* 2/3 5/4) 5/6))

(t/is (= (rationalize 1.25) 5/4))
(t/is (= (rationalize -2/4) -1/2))
(t/is (= (rationalize 2) 2))
(t/is (= (rationalize 2.0) 2N))

                                        ; Promotions

(t/is (instance? BigInt (inc' java.lang.Long/MAX_VALUE)))
(t/is (= (dec' java.lang.Long/MIN_VALUE) -9223372036854775809N))
(t/is (= (take 8 (str (apply *' (range 1 101)))) (seq (str 93326215))))

                                        ; Corner cases

(t/is (= (/ java.lang.Long/MIN_VALUE -1) (+ java.lang.Long/MAX_VALUE 1N)))
(t/is (= (rem java.lang.Long/MIN_VALUE -1) 0))

                                        ; Strings

(t/is (.startsWith "small string" "sm") )
(t/is (.startsWith "large string123456789abcdefghijk" "la"))


                                        ; Sorting and Comparing
(t/is (= (compare ##NaN 1) 0))
(t/is (= (compare nil 1) -1))
(t/is (= (compare nil nil) 0))

(t/is (= (<= ##NaN 0) false))
(t/is (= (= 1 1.0) false))
(t/is (= (== 1 1.0 1N) true))
(t/is (= (= 0.5 1/2) false))
(t/is (= (== 0.5 1/2 2/4) true))
(t/is (= (= ##NaN ##NaN) false))

(t/is (= (compare (first {:a 1}) [:a 1]) 0))
(t/is (= (compare (first {:a 1}) [:a 2]) -1))
(t/is (= (first {:a 1}) [:a 1]))

(t/is (= (compare "abc" "def") -3))

(t/is (= (compare 0N 0) 0))
(t/is (= (compare -10N -10) 0))
(t/is (= (compare 100000000000000000000N 1000000N) 1))
(t/is (= (compare 256 -1000000000000000000000N) 1))
(t/is (= (compare 10000000000000000000000N 1000000000000) 1))
(t/is (= (compare 1000000000000 1000000000000N) 0))

(t/is (= (sort '( 5 9 ##Inf nil -10.0 )) '(nil -10.0 5 9 ##Inf)))
(t/is (= (sort-by count [ "ab" "" "zab" "cada"]) '("" "ab" "zab" "cada")))

                                        ; Lists, Vectors and Sequences

(t/is (= (conj '() :a :b) '(:b :a)))
(t/is (= (conj [] :a :b) [:a :b]))
(t/is (= (cons :x '()) '(:x)))

(t/is (= '() []))
(t/is (= '( 1 ) [ 1 ]))
(t/is (not= '() #{}))
(t/is (= (seq '( 1 2 )) (seq #{ 1 2 })))

(t/is (= (rseq [ 1 2 3 4 ]) '( 4 3 2 1 )))
(t/is (= (rseq [ :a :b ]) '( :b :a )))

                                        ; str

(t/is (= (str \a \b \c) "abc"))
(t/is (= (str "ab" "cd") "abcd"))
(t/is (= (str [nil]) "[nil]"))
(t/is (= (str '( 1 )) "(1)"))
(t/is (= (str nil nil) ""))
(t/is (= (str :ab) ":ab"))

                                        ; Lazy-seqs and Delays

(t/is (= (range 5) '( 0 1 2 3 4 )))
(t/is (= (last (take 1000 (range))) 999))
(t/is (= '() (lazy-seq '())))

(t/is (= @(delay :a) :a))
(t/is (not (realized? (delay :a))))

                                        ; Maps and Sets

(t/is (= (assoc {} :a 1 :b 1) { :a 1 :b 1 }))
(t/is (= (first { :a 1 :b 1 }) [ :a 1 ]))
(t/is (= (count (into #{} (range 50))) 50))

                                        ; Dates

(t/is (instance? java.util.Date #inst "2024-01-01"))
(t/is (= (.getTime #inst "2024-01-01") 1704067200000))

                                        ; Queues

(def Q (conj clojure.lang.PersistentQueue/EMPTY :a :b :c :d))

(t/is (not (empty? Q)))
(t/is (= (peek Q) :a))
(t/is (= (seq (pop Q)) '( :b :c :d)))

                                        ; Regex

(t/is (= (re-find #"^\d+$" "123") "123"))
(t/is (= (re-find #"^\d+$" "abc123") nil))
(t/is (= (re-find #"\w{2}" "abc") "ab"))

                                        ; Formatting

(t/is (= (format "%.2f" 0.123) "0.12"))

                                        ; Bit-operations

(t/is (= (bit-shift-left 1 16) 65536))
(t/is (= (bit-shift-right 65536 16) 1))

                                        ; Hashes

(t/is (= (hash 0) (hash nil) 0))
(t/is (= (hash '()) (hash []) -2017569654))
(t/is (= (hash #{}) (hash {}) -15128758))
(t/is (= (hash [ 1 2 3 ]) 736442005))
(t/is (= (hash false) 1237))
(t/is (= (hash true) 1231))
(t/is (= (hash 1) (hash 1N) 1392991556))
(t/is (= (hash "mämmi") 2059946131))
(t/is (= (hash \a) 97))
(t/is (= (hash 1.1) -1503133693))
(t/is (= (hash #inst "2014-01-01") 1259963715))
(t/is (= (hash #{ 1 }) 1038464948))
(t/is (= (hash { "A" 1 "B" 2 }) -457774292))
(t/is (= (hash (clojure.java.io/file "/")) 1234366))
(t/is (= (hash 'ab/cd) 815946391))
(t/is (= (hash :ääää) -1406749036))
; (t/is (= (hash :fish/dish) 1445185017))
(t/is (= (hash (+' Long/MAX_VALUE 1)) -2147483648))
(t/is (= (hash 1000000000000000000000N) 1645008273))
(t/is (= (hash 1/2) 3))
(t/is (= (hash 1000000000000000000000/33333) 1644976036))

                                        ; Special Functions

(t/is (= ((juxt numerator denominator) 2/3) [ 2 3 ]))
