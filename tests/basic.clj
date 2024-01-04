                                        ; Primitives
(is (integer? 1))
(is (number? 1.23))

(is (instance? Boolean true))
(is (instance? Boolean false))
(is (instance? Long 1))
(is (instance? Codepoint \a))
(is (instance? EmptyList '()))
(is (instance? Double 0.1))
(is (instance? Byte (byte 1)))
(is (instance? Short (short 1)))
(is (instance? Integer (int 1)))

(is (instance? Keyword :abc))
(is (instance? Symbol 'abc))
(is (= "bb" (name :aa/bb)))
(is (= "aa" (namespace 'aa/bb)))

(is (instance? Double ##Inf))
(is (instance? Double ##-Inf))
(is (instance? Double ##NaN))

                                        ; Other numeric types
(is (ratio? 1/2))
(is (instance? BigInt 1N))

                                        ; Arithmetics

(is (= 6 (+ 1 2 3)))
(is (= 1/2 (/ 1 2)))
(is (= (rationalize 1.25) 5/4))
(is (= (* 2/3 5/4) 5/6))

(is (= (abs -100N) 100))

(is (= (inc 5) 6))
(is (= (dec 6) 5))

                                        ; Promotions

(is (instance? BigInt (inc' java.lang.Long/MAX_VALUE)))
(is (= (dec' java.lang.Long/MIN_VALUE) -9223372036854775809N))

                                        ; Corner cases

(is (= (/ java.lang.Long/MIN_VALUE -1) (+ java.lang.Long/MAX_VALUE 1N)))
(is (= (rem java.lang.Long/MIN_VALUE -1) 0))

                                        ; Strings

(is (.startsWith "small string" "sm") )
(is (.startsWith "large string123456789abcdefghijk" "la"))


                                        ; Sorting and Comparing
(is (= (compare ##NaN 1) 0))
(is (= (compare nil 1) -1))
(is (= (compare nil nil) 0))

(is (= (<= ##NaN 0) false))
(is (= (= 1 1.0) false))
(is (= (== 1 1.0 1N) true))
(is (= (= 0.5 1/2) false))
(is (= (== 0.5 1/2 2/4) true))
(is (= (= ##NaN ##NaN) false))

(is (= (sort '( 5 9 ##Inf nil -10.0 )) '(nil -10.0 5 9 ##Inf)))
(is (= (sort-by count [ "ab" "" "zab" "cada"]) '("" "ab" "zab" "cada")))

                                        ; Lists, Vectors and Sequences

(is (= (conj '() :a :b) '(:b :a)))
(is (= (conj [] :a :b) [:a :b]))
(is (= (cons :x '()) '(:x)))

(is (= '() []))
(is (= '( 1 ) [ 1 ]))
; (is (not= '() #{}))

(is (= (rseq [ 1 2 3 4 ]) '( 4 3 2 1 )))
(is (= (rseq [ :a :b ]) '( :b :a )))

                                        ; str

(is (= (str \a \b \c) "abc"))
(is (= (str "ab" "cd") "abcd"))
(is (= (str [nil]) "[nil]"))
(is (= (str '( 1 )) "(1)"))
(is (= (str nil nil) ""))
(is (= (str :ab) ":ab"))

                                        ; Lazy-seqs and Delays

(is (= (range 5) '( 0 1 2 3 4 )))
(is (= (last (take 1000 (range))) 999))
(is (= '() (lazy-seq '())))

(is (= @(delay :a) :a))
(is (not (realized? (delay :a))))

                                        ; Maps and Sets

(is (= (assoc {} :a 1 :b 1) { :a 1 :b 1 }))
(is (= (first { :a 1 :b 1 }) [ :a 1 ]))

(is (= (count (into #{} (range 50))) 50))

                                        ; Dates

(is (instance? java.util.Date #inst "2024-01-01"))
(is (= (.getTime #inst "2024-01-01") 1704067200000))

                                        ; Queues

(def Q (conj clojure.lang.PersistentQueue/EMPTY :a :b :c :d))

(is (not (empty? Q)))
(is (= (peek Q) :a))
(is (= (seq (pop Q)) '( :b :c :d)))

                                        ; Regex

(is (= (re-find #"^\d+$" "123") "123"))
(is (= (re-find #"^\d+$" "abc123") nil))
(is (= (re-find #"\w{2}" "abc") "ab"))

                                        ; Formatting

(is (= (format "%.2f" 0.123) "0.12"))

                                        ; Bit-operations

(is (= (bit-shift-left 1 16) 65536))
(is (= (bit-shift-right 65536 16) 1))

                                        ; Hashes

(is (= (hash '()) -2017569654))
(is (= (hash false) 1237))
(is (= (hash true) 1231))
(is (= (hash 1) 1392991556))
(is (= (hash "a") 1455541201))
