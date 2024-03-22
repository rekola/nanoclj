(require '[ clojure.test :as t ]
         'clojure.java.io
         'clojure.string)
                                        ; Primitives
(t/is (integer? 1))
(t/is (number? 1.23))

(t/is (instance? Boolean true))
(t/is (instance? Boolean false))
(t/is (instance? Long 1))
(t/is (instance? nanoclj.lang.Codepoint \a))
(t/is (instance? nanoclj.lang.EmptyList '()))
(t/is (instance? Double 0.1))
(t/is (instance? Byte (byte 1)))
(t/is (instance? Short (short 1)))
(t/is (instance? Integer (int 1)))

(t/is (instance? clojure.lang.Keyword :abc))
(t/is (instance? clojure.lang.Symbol 'abc))
(t/is (= "bb" (name :aa/bb)))
(t/is (= "aa" (namespace 'aa/bb)))

(t/is (instance? Double ##Inf))
(t/is (instance? Double ##-Inf))
(t/is (instance? Double ##NaN))

(t/is (.equals 0.0 0.0))
(t/is (not (.equals 0.0 -0.0)))

                                        ; Boxed Longs

(t/is (not (= 0 java.lang.Long/MAX_VALUE)))

                                        ; Arithmetics

(t/is (= 6 (+ 1 2 3)))

(t/is (= (abs -100N) (abs -100) 100))
(t/is (= (abs Integer/MIN_VALUE) 2147483648))

(t/is (= (inc 5) 6))
(t/is (= (dec 6) 5))
(t/is (= (dec Integer/MIN_VALUE) -2147483649))
(t/is (= (inc Integer/MAX_VALUE) 2147483648))

(t/is (= (+ 1/2 1/2) 1))
(t/is (= (- 10 10/3) 20/3))

(t/is (= (min 100 10) 10))
(t/is (= (max 100 10) 100))
(t/is (= (min -10N 10) -10N))
(t/is (= (max -10000000000000000000000000000N 0) 0))
                                        ; Bigints

(t/is (instance? clojure.lang.BigInt 1N))
(t/is (= (* 1000000000N 1000000000N) 1000000000000000000N))
(t/is (= (* 100000000000000000000 100000000000000000000)
       10000000000000000000000000000000000000000))
(t/is (= (/ 100000000000000000000000000 1000000000000000000000000) 100N))

                                        ; Ratios

(t/is (ratio? 1/2))
(t/is (instance? Long 2/1))
(t/is (instance? clojure.lang.BigInt 10000000000000000000/2))

(t/is (= 1/2 (/ 1 2)))
(t/is (= (* 2/3 5/4) 5/6))

(t/is (= (rationalize 1.25) 5/4))
(t/is (= (rationalize -2/4) -1/2))
(t/is (= (rationalize 2) 2))
(t/is (= (rationalize 2.0) 2N))

                                        ; Promotions

(t/is (instance? clojure.lang.BigInt (inc' java.lang.Long/MAX_VALUE)))
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

(t/is (= (nth [ 1 2 3 4 ] 0) 1))
(t/is (= (nth [ 1 2 3 4 ] 1000 :not-found) :not-found))
(t/is (= (nth '( 1 2 3 4 ) 3) 4) 4)
(t/is (= (nth '( 1 2 3 4 ) 1000 :not-found) :not-found))
(t/is (= (nth "こんにちは" 4) \は))
(t/is (= (nth "こんにちは" 5 :not-found) :not-found))

(t/is (= (vector-of :boolean true false true false) [ true false true false ]))
(t/is (= (vector-of :byte 1.0 2.0 3.0) [ 1 2 3 ]))
(t/is (= (vector-of :short 1.0 2.0 3.0) [ 1 2 3 ]))
(t/is (= (vector-of :int 1.0 2.0 3.0) [ 1 2 3 ]))
(t/is (= (vector-of :float 1.0 2.0 3.0) [ 1.0 2.0 3.0 ]))
(t/is (= (vector-of :double 1.0 2.0 3.0) [ 1.0 2.0 3.0 ]))

                                        ; Printing and str

(t/is (= (str \a \b \c) "abc"))
(t/is (= (str "ab" "cd") "abcd"))
(t/is (= (str [nil]) "[nil]"))
(t/is (= (str '( 1 )) "(1)"))
(t/is (= (str 1N) "1"))
(t/is (= (str nil nil) ""))
(t/is (= (str :ab) ":ab"))
(t/is (= (str ["a"]) "[\"a\"]"))
(t/is (= (str String) "class java.lang.String"))

(t/is (= (print-str nil) (pr-str nil) "nil"))
(t/is (= (print-str ["a"]) "[a]"))
(t/is (= (pr-str ["a"]) "[\"a\"]"))

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
(t/is (= (conj { :a 1 } [ :b 2 ]) { :a 1 :b 2}))
(t/is (= (conj #{ :a } :b ) #{ :a :b }))
(t/is (= (conj { :a 1 } { :b 2 }) { :a 1 :b 2 }))
(t/is (= (assoc [ :a :b :c :d ] 0 :A) [ :A :b :c :d ]))
(t/is (= (set '[1 2 3 4]) #{ 1 2 3 4 }))

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
(t/is (= (re-find #"^\pL{3}$" "äää") "äää"))

                                        ; Formatting

(t/is (= (format "%.2f" 0.123) "0.12"))
(t/is (= (format "%s" nil) "null"))

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
(t/is (= (hash (clojure.java.io/as-url "file:///abc")) 4639566))
(t/is (= (hash 'ab/cd) 815946391))
(t/is (= (hash 'ääää) 233782491))
; (t/is (= (hash :fish/dish) 1445185017))
(t/is (= (hash (+' Long/MAX_VALUE 1)) -2147483648))
(t/is (= (hash 1000000000000000000000N) 1645008273))
(t/is (= (hash 1/2) 3))
(t/is (= (hash 1000000000000000000000/33333) 1644976036))
(t/is (= (hash (parse-uuid "4fe5d828-6444-11e8-8222-720007e40350")) -1368934256))
(t/is (= (hash (Math/sqrt -1)) 2146959360))

                                        ; Special Functions

(t/is (= ((juxt numerator denominator) 2/3) [ 2 3 ]))
(t/is (= (interleave [ :a :b :c ] (range)) '( :a 0 :b 1 :c 2 )))
(t/is (= ((fnil identity :a) 1) 1))
(t/is (= ((fnil identity :a) nil) :a))
                                        ; Namespaces

(t/is (resolve 'clojure.string/upper-case))

                                        ; Control

(t/is (= (with-out-str (dotimes [n 4] (print "X"))) "XXXX"))
(t/is (= (loop [a 4 b a] (if (zero? b) 1000 (recur a (dec b)))) 1000))
(t/is (= (loop []) nil))

                                        ; Arrays

(t/is (= (seq (boolean-array '( false true false true))) '( false true false true )))
(t/is (= (alength (float-array 1000000)) 1000000))
(t/is (= (seq (to-array [ :a :b :c :d ])) '( :a :b :c :d )))
(t/is (= (seq (into-array [ 1.0 2.0 3.0 4.0 ])) '( 1.0 2.0 3.0 4.0 )))

                                        ; UUIDs

(t/is (= (.version (random-uuid)) 4))
(t/is (= (.variant (random-uuid)) 2))
