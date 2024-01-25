(require '[ clojure.test :as t ]
         '[ clojure.xml :as xml ])

(t/is (= (xml/parse (char-array "<?xml version=\"1.0\"?>\n<root></root>")) { :tag :root }))

