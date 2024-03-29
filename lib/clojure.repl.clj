(ns clojure.repl
  "The REPL")
(require 'clojure.string 'clojure.java.io 'linenoise)

(defn- autocomplete-strings
  [prefix s strings] (loop [acc [] ss strings]
                       (cond (empty? ss) acc
                             (.startsWith (first ss) s) (recur (conj acc (str prefix (first ss))) (rest ss))
                             :else (recur acc (rest ss)))))

(defn- autocomplete-syntax
  [prefix s] (autocomplete-strings prefix s [ "if" "let" "fn" "quote" "def" "do" "cond" "lazy-seq" "and" "or" "macro" "try" "loop" "dotimes" "thread" ] ))

(defn- autocomplete-envir
  [envir prefix s coll] (if (nil? envir) coll
                            (let [f (first envir)
                                  r (autocomplete-envir (next envir) prefix s coll)
                                  ]
                              (loop [r r f f]
                                (cond (empty? f) r
                                      (.startsWith (str (key (first f))) s) (recur (conj r (str prefix (key (first f)))) (rest f))
                                      :else (recur r (rest f)))))))

(defn- autocomplete-expr
  [prefix s] (autocomplete-envir *env* prefix s (autocomplete-syntax prefix s)))

(defn- autocomplete-str
  [prefix s] (autocomplete-strings prefix s [ "http://" "https://" "ftp://" ]))

(defn- add-space
  [completions] (if (= (count completions) 1) [ (str (first completions) \space ) ] completions))

(defn *autocomplete-hook*
  [s] (let [in-str (re-find #"^((?:[^\"]*\"[^\"]*\")*\")([^\"]*)$" s)
            in-expr (re-find #"^(.*[\s\(])?(\S*)$" s)
            completions (cond in-str (autocomplete-str (in-str 1) (in-str 2))
                              in-expr (add-space (autocomplete-expr (in-expr 1) (in-expr 2)))
                              :error [])
            ]
        (if (empty? completions) [ s ] completions)))

(defn source
  "Returns the source for a macro or function"
  [x] (if (or (closure? x) (macro? x)) (cons 'fn (first x)) nil))

(def-macro (doc name)
  (println "-------------------------")
  `(let ((M (meta (var ~name))))
     (println (str (:ns M)) \/ (:name M))
     (println (:doc M))))

(defn apropos
  "Returns a list of functions whose name contains str"
  [pattern] (filter (fn [s] (clojure.string/includes? (str s) pattern)) (ns-interns root)))

(defn- draw-banner
  [label version] (if (*out* :graphics)
                    (let [v-margin 5
                          h-margin 0
                          [cell-width cell-height] (:cell-size *out*)
                          s1 (* 1.5 cell-height)
                          s2 (* 0.6 cell-height)
                          cx (clojure.java.io/writer 0 0 :gray)]
                      (binding [*out* cx]
                        (set-font-size s1)
                        (let [[ label-w label-h ] (get-text-extents label)
                              y-pos (* 1.3 cell-height)]
                          (resize (* 2 (+ h-margin label-w)) (* 2 cell-height))
                          (move-to h-margin y-pos)
                          (set-font-size s1)
                          (text-path label)
                          (set-font-size s2)
                          (text-path version)
                          (set-color 0.8)
                          (fill)

                          (set-line-width :hair)
                          (move-to h-margin y-pos)
                          (set-font-size s1)
                          (text-path label)
                          (set-color 1)
                          (stroke)
                          (flush)))
                      cx)
                    (str label \space version)))


(defn repl
  "Starts the REPL"
  []

  (mode :block)
  (print (draw-banner "nanoclj" (System/getProperty "nanoclj.version")))
  (newline)
  
  (let [hfn (clojure.java.io/file (System/getProperty "user.home") "/.nanoclj_history")
        f (fn [] (let [prompt (str *ns* "=> ")
                       line (linenoise/read-line prompt)
                       core-ns (find-ns 'clojure.core)]
                   (if line
                     (do
                       (linenoise/history-append line hfn)
                       (save)
                       (set-color (if (= *theme* :dark)
                                    [0.6 0.6 0.6]
                                    [0.4 0.4 0.4]))
                       (println prompt line)
                       (restore)
                       (try (let [r (load-string line)]
                              (prn r)
                              (set! *3 *2)
                              (set! *2 *1)
                              (set! *1 r)
                              (recur))
                            (catch java.lang.Throwable e
                              (set! *e e)
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
