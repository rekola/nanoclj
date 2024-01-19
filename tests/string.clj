(require '[ clojure.test :as t ]
         '[ clojure.string :as str ])

(t/is (str/blank? "    "))
(t/is (not (str/blank? "abc")))

(t/is (= (str/upper-case "abc") "ABC"))
(t/is (= (str/lower-case "ÄÄÄ") "äää"))
(t/is (= (str/capitalize "michael") "Michael"))

(t/is (= (str/index-of "Hélen" "len") 2))

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
