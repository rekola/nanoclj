(require '[ clojure.test :as t ]
         '[ clojure.data.csv :as csv ]
         '[ clojure.java.io :as io ])

(def rdr (io/reader (char-array "1,2,3,4\n")))
(def d (csv/read-csv rdr))
(t/is (= d '( [ "1" "2" "3" "4" ])))
