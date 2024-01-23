(require '[ clojure.test :as t ]
         '[ clojure.string :as str ])

(t/is (str/blank? "    "))
(t/is (not (str/blank? "abc")))

(t/is (= (str/upper-case "abc") "ABC"))
(t/is (= (str/lower-case "ÄÄÄ") "äää"))
(t/is (= (str/capitalize "michael") "Michael"))

(t/is (= (str/index-of "Hélen Hélen" "len") 2))
(t/is (= (str/last-index-of "Hélen Hélen" "len") 8))

(t/is (= (str/triml "   xxx   ") "xxx   "))
(t/is (= (str/trimr "   xxx   ") "   xxx"))
(t/is (= (str/trim "   xxx   ") "xxx"))

(def s "Ångström second third  ")
(t/is (= (str/split s #"\s+") ["Ångström" "second" "third"]))
(t/is (= (str/split s #"\s+" -1) ["Ångström" "second" "third" ""]))
(t/is (= (str/split s #"\s+" 1) ["Ångström second third  "]))
(t/is (= (str/split s #"\s+" 2) ["Ångström" "second third  "]))
(t/is (= (str/split s #"\s+" 3) ["Ångström" "second" "third  "]))
(t/is (= (str/split s #"\s+" 4) ["Ångström" "second" "third" ""]))

(t/is (= (str/replace-first s #"\bthird\b" "fourth") "Ångström second fourth  "))
(t/is (= (str/replace s #"\pL+" "xxx") "xxx xxx xxx  "))

(t/is (= (str/split-lines "test \n string") ["test " " string"]))

(t/is (= (str/escape "Rock & roll! <3" {\& "&amp;", \< "&lt;"}) "Rock &amp; roll! &lt;3"))
(t/is (= (str/escape "123" {\1 "2", \2 "3", \3 "4"}) "234"))
