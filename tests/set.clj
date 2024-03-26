(ns test.set)
(require '[ clojure.test :as t ]
         '[ clojure.set :as s ])

(t/is (= (s/union #{ 1 } #{ 2 }) #{ 1 2 }))

(t/is (= (s/intersection #{ 1 2 3 4 5 } #{ 4 5 6 7 }) #{ 4 5 }))
(t/is (= (s/intersection #{1 :a} #{:a 3} #{:a}) #{ :a }))
(t/is (= (s/intersection #{:a :b :c } #{:b :c :e} #{:f}) #{}))

(t/is (= (s/difference #{ "One" "Two" "Three" "Four" } #{ 1 2 "Three" } #{ "Four"}) #{ "Two" "One" }))

(t/is (s/superset? #{ :a :b :c :d } #{ :a }))
(t/is (not (s/superset? #{ :a :b :c :d } #{ 1 2 })))

(t/is (= (s/select odd? (into #{} (range 10))) #{ 1 3 5 7 9 }))
