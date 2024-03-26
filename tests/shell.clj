(ns test.shell)
(require '[ clojure.test :as t ]
         '[ clojure.java.shell :as sh ])

(t/is (= (sh/sh "echo 1") {:out "1\n" :exit 0 :err nil}))
