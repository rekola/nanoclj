(ns clojure.java.shell
  "Shell operations")
(require 'clojure.java.io)

(defn sh
  "Launch a process and capture output"
  [arg & args] (let [rdr (clojure.java.io/reader (str "|" (clojure.string/join \space (cons arg args))))
                     out (loop [ s "" c (.read rdr) ] (if (= (int c) -1) s (recur (conj s (char c)) (.read rdr))))
                     rv (-close rdr)
                     ]
                 {:out out
                  :exit rv
                  :err nil}))

