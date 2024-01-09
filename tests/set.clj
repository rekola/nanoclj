(require '[ clojure.test :as t ]
         '[ clojure.set :as s ])

(t/is (= (s/union #{ 1 } #{ 2 }) #{ 1 2 }))

(t/is (= (s/intersection #{ 1 2 3 4 5 } #{ 4 5 6 7 }) #{ 4 5 }))
(t/is (= (s/intersection #{1 :a} #{:a 3} #{:a}) #{ :a }))
(t/is (= (s/intersection #{:a :b :c } #{:b :c :e} #{:f}) #{}))
