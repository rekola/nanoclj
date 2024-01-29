(require '[ clojure.test :as t ]
         '[ clojure.math.numeric-tower :as towr ])

(t/is (= (towr/expt 2 32) 4294967296))
(t/is (= (towr/expt 1/2 32) 1/4294967296))
(t/is (= (towr/expt 1000N 10N) 1000000000000000000000000000000N))

(t/is (= (towr/gcd 15 10) 5))
