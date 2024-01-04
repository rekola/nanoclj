(is (clojure.string/blank? "    "))
(is (not (clojure.string/blank? "abc")))

(is (= (clojure.string/upper-case "abc") "ABC"))
(is (= (clojure.string/lower-case "ÄÄÄ") "äää"))
(is (= (clojure.string/capitalize "michael") "Michael"))

(is (= (clojure.string/index-of "Hélen" "len") 2))
