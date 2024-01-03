                                        ; Primitives

(is (integer? 1))
(is (ratio? 1/2))
(is (number? 1.23))

                                        ; Arithmetics

(is (= 6 (+ 1 2 3)))
(is (= 1/2 (/ 1 2)))
(is (= (rationalize 1.25) 5/4))

                                        ; Strings

(is (.startsWith "small string" "sm") )
(is (.startsWith "large string123456789abcdefghijk" "la"))


