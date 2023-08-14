(def clojure.repl (package

(defn source
  "Returns the source for a macro or function"
  [x] (if (or (closure? x) (macro? x)) (cons 'fn (car x)) false))

(def-macro (doc name)
  (println "-------------------------")
  `(do
     (println ',name)
     (println (:doc (meta ,name)))))

(defn apropos [pattern]
  "Returns a list of functions whose name contains str"
  (filter (fn [s] (clojure.string/includes? (str s) pattern)) (symbols root)))
    
(defn repl
  "Starts the REPL"
  [] (print "user> ")
     (let [r (eval (read-string (read-line)))]
       (prn r)
       (repl)))

))
