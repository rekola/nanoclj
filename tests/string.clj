(require '[ clojure.test :as t ])

(t/is (clojure.string/blank? "    "))
(t/is (not (clojure.string/blank? "abc")))

(t/is (= (clojure.string/upper-case "abc") "ABC"))
(t/is (= (clojure.string/lower-case "ÄÄÄ") "äää"))
(t/is (= (clojure.string/capitalize "michael") "Michael"))

(t/is (= (clojure.string/index-of "Hélen" "len") 2))
