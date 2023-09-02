(def clojure.repl (package

(defn source
  "Returns the source for a macro or function"
  [x] (if (or (closure? x) (macro? x)) (cons 'fn (car x)) nil))

(def-macro (doc name)
  (println "-------------------------")
  `(do
     (println ',name)
     (println (:doc (meta ,name)))))

(defn apropos
  "Returns a list of functions whose name contains str"
  [pattern] (filter (fn [s] (clojure.string/includes? (str s) pattern)) (symbols root)))
    
(defn repl
  "Starts the REPL"
  [] (let [prompt "user> "
           line (linenoise/read-line prompt)]
       (if line (let [r (try (eval (read-string line)))]
                  (prn r)
                  (when (defined? '*2) (def *3 *2))
                  (when (defined? '*1) (def *2 *1))
                  (def *1 r)
                  (recur))
           nil)))

))
