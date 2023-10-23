(in-ns 'clojure.repl)

(defn source
  "Returns the source for a macro or function"
  [x] (if (or (closure? x) (macro? x)) (cons 'fn (car x)) nil))

(def-macro (doc name)
  (println "-------------------------")
  `(do
     (println ',name)
     (println (:doc (meta (var ,name))))))

(defn apropos
  "Returns a list of functions whose name contains str"
  [pattern] (filter (fn [s] (clojure.string/includes? (str s) pattern)) (ns-interns root)))
    
(defn repl
  "Starts the REPL"
  []
;  (in-ns 'user)
  (let [hfn (clojure.java.io/file (System/getProperty "user.home") "/.nanoclj_history")
        f (fn [] (let [prompt (str (:name (meta *ns*)) "=> ")
                       line (linenoise/read-line prompt)]
                   (if line
                     (do
                       (linenoise/history-add line)
                       (linenoise/history-save hfn)
                       (try (let [r (load-string line)]
                              (prn r)
                              (when (defined? '*2) (def *3 *2))
                              (when (defined? '*1) (def *2 *1))
                              (def *1 r)
                              (recur))
                            (catch java.lang.Throwable e
                              (def *e e)
                              (save)
                              (set-color [ 0.85 0.31 0.3 ])
                              (println e)
                              (restore)
                              (newline)
                              (recur)))
                       nil))))]
    (linenoise/history-load hfn)
    (f)
    ))
