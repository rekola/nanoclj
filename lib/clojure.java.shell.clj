(ns clojure.java.shell)

(defn sh
  "Launch a process and capture output"
  [arg & args] (let [rdr (clojure.java.io/reader (str "|" (clojure.string/join \space (cons arg args))))
                     f (fn [s] (let [c (-read rdr)] (if (= c ##Eof) s (recur (conj s c)))))
                     out (f "")
                     rv (-close rdr)
                     ]
                 {:out out
                  :exit rv
                  :err nil}))

