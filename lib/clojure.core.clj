; Utilities for types.

(defn vector? [x] (instance? clojure.lang.PersistentVector x))
(defn keyword? [x] (instance? clojure.lang.Keyword x))
(defn delay? [x] (instance? clojure.lang.Delay x))
(defn seq? [x] (is-any-of? (type x) scheme.lang.Seq clojure.lang.PersistentList))
(defn map-entry? [x] (instance? clojure.lang.MapEntry x))
(defn set? [x] (instance? clojure.lang.PersistentTreeSet x))
(defn map? [x] (instance? clojure.lang.PersistentArrayMap x))
(defn coll? [x] (is-any-of? (type x)
                            clojure.lang.PersistentTreeSet
                            clojure.lang.PersistentArrayMap
                            clojure.lang.PersistentVector
                            clojure.lang.PersistentList
                            scheme.lang.Seq
                            clojure.lang.Cons
                            ))
(defn associative? [coll] (is-any-of? (type x)
                                      clojure.lang.PersistentVector
                                      clojure.lang.PersistentArrayMap))

(def keyword clojure.lang.Keyword)
(def char java.lang.Character)
(def short java.lang.Integer)
(def double java.lang.Double)
(def float java.lang.Double)

; Casts argument to int
(def int java.lang.Integer)

; Casts argument to long (or int if it is sufficient)
(def long java.lang.Long)

(defn num
  "Casts argument to number"
  [x] (if (number? x) x (long x)))

(defn vec [coll] (reduce conj- [] coll))
(defn set [coll] (reduce conj- #() coll))

(defn run! [proc coll] (if (empty? coll) nil (do (proc (first coll)) (run! proc (rest coll)))))

; Utilities for math.

(defn even?
  "Returns true if the argument is even"
  [n] (zero? (mod n 2)))

(defn odd?
  "Returns true if the argument is odd"
  [n] (not (even? n)))

(defn pos? [n] (> n 0))
(defn neg? [n] (< n 0))
(defn pos-int? [n] (and (integer? n) (> n 0)))
(defn neg-int? [n] (and (integer? n) (< n 0)))
(defn nat-int? [n] (and (integer? n) (>= n 0)))
(defn float? [n] (instance? java.lang.Double x))
(defn double? [n] (instance? java.lang.Double x))
(defn NaN? [n] (identical? n ##NaN))
(defn infinite? [n] (or (== n ##Inf) (== n ##-Inf)))

(defn mod [num div] (let [m (rem num div)] 
                      (if (or (zero? m) (= (pos? num) (pos? div)))
                        m
                        (+ m div))))

(defn abs
  "Returns the absolute value of n"
  [n] (if (>= n 0) n (- n)))

(defn ratio?
  "Returns true if n is a Ratio"
  [n] (instance? clojure.lang.Ratio n))

(defn rational?
  "Returns true if n is an integer or a Ratio"
  [n] (or (integer? n) (instance? clojure.lang.Ratio n)))

(defn numerator
  "Returns the numerator of a Ratio"
  [n] (if (ratio? n) (first n) n))

(defn denominator
  "Returns the denominator of a Ratio"
  [n] (if (ratio? n) (rest n) 1))

(defn quot
  "Returns the quotinent of dividing a by b"
  [a b] (long (/ a b)))

; Miscellaneous

(defn new [t & args]
  "Creates an object"
  (apply* t args))

(defn identity [x] x)

(def count
  "Counts the number of elements in coll. For vectors, sets and maps it's O(1), but for
  lists and sequences it is O(n)"
  (fn [coll] (if (or (vector? coll) (set? coll) (map? coll))
               (long coll)
               (if (empty? coll)
                 0
      	         (loop [accum 1 coll (next coll)]
                   (if coll
                     (recur (inc accum) (next coll))
                     accum))))))
                     

(def some (fn [pred coll] (cond (empty? coll) false
                                (pred (first coll)) true
				:else (some pred (rest coll)))))
(def-macro (time body)
   `(let [ t0 (System/currentTimeMillis) ]
        (let ((e ,body))
	     (println (str "Elapsed time: " (- (System/currentTimeMillis) t0) " msecs"))
	     e)))

(def-macro (comment body) nil)

(defn constantly
  "Creates a function that always returns x and accepts any number of arguments"
  [x] (fn [& args] x))


; Strings

(def subs (fn ([s start]     (str (drop start s)))
     	      ([s start end] (str (take (- end start) (drop start s))))))


; Sequences

(def concat (fn
              ([] '())
              ([x] x)
              ([x y] (if (empty? x) y (cons (first x) (concat (rest x) y))))))

(def iterate (fn [f x] (cons x (lazy-seq (iterate f (f x))))))

(def repeat (fn ([x]   (cons x (lazy-seq (repeat x))))
                ([n x] (if (<= n 0) '() (cons x (repeat (dec n) x))))))

(defn last
  "Returns the last element of sequence"
  [s] (if (next s) (recur (next s)) (first s)))

(defn last-2
  "Returns the last element of sequence"
  [[f & r]] (do
              (println "f = " f ", r = " r)
              (if r (recur r) f))
  )

(def butlast (fn [coll] (if (next coll)
                          (cons (first coll) (recur (next coll)))
                          '())))

(defn reverse [coll] (reduce conj- '() coll))

(defn interpose [sep coll] (if (empty? coll) '() (cons (first coll) (if (empty? (rest coll)) '() (cons sep (interpose sep (rest coll)))))))

(def interleave (fn
                  ([] '())
                  ([c1] c1)
                  ([c1 c2] (if (or (empty? c1) (empty? c2)) '()
                               (cons (first c1) (cons (first c2)
                                     (lazy-seq (interleave (rest c1) (rest c2)))))))
                  (c1 c2 c3) (if (or (empty? c1) (empty? c2) (empty? c3)) '()
                                 (cons (first c1) (cons (first c2) (cons (first c3)
                                       (lazy-seq (interleave (rest c1) (rest c2) (rest c3)))))))))

(defn take-while [pred coll] (cond (empty? coll) '()
                                   (pred (first coll)) (cons (first coll) (lazy-seq (take-while pred (rest coll))))
                                   :else '()))

(defn drop-while
  "Drops elements until pred is false"
  [pred coll] (cond (empty? coll) '()
                    (pred (first coll)) (recur pred (rest coll))
                    :else coll))

(defn take-last [n coll] (let [cnt (count coll)] (if (< cnt n) coll (drop n coll))))
(defn drop-last [n coll] (let [cnt (count coll)] (if (< cnt n) '() (take (- cnt n) coll))))

(defn ffirst [x] (first (first x)))
(defn fnext [x] (first (next x)))
(defn nnext [x] (next (next x)))
(defn nfirst [x] (next (first x)))

; Stacks

(def peek (fn [coll] (cond (empty? coll) '()
			   (vector? coll) (coll (dec (count coll)))
                           :else (first coll))))
(def pop (fn [coll] (cond (empty? coll) '()
     	     	    	  (vector? coll) (butlast coll)
			  :else (rest coll))))


(def range (fn ([]               (iterate inc 0))
               ([end]            (range 0 end))
               ([start end]      (if (ge start end) '() (cons start (lazy-seq (range (inc start) end)))))
     	       ([start end step] (if (ge start end) '() (cons start (lazy-seq (range (+ start step) end step)))))
             ))
                                       
(defn doall [seq] (if (empty? seq) '() (cons (first seq) (doall (rest seq)))))
(defn dorun [seq] (if (empty? seq) nil (recur (rest seq))))

(defn take [n coll] (if (or (<= n 0) (empty? coll)) '() (cons (first coll) (lazy-seq (take (dec n) (rest coll))))))

(defn drop [n coll] (if (zero? n) coll (recur (dec n) (rest coll))))

(def map
  "Returns a lazy sequence with each element mapped using f"
  (fn [f coll] (if (empty? coll) '() (cons (f (first coll)) (lazy-seq (map f (rest coll)))))))

(def repeatedly (fn ([f]   (cons (f) (lazy-seq (repeatedly f))))
                    ([n f] (if (<= n 0) '() (cons (f) (lazy-seq (repeatedly (dec n) f)))))))

; Maps & Sets

(def key first)
(def val rest)

(defn keys
  "Returns the values in the map" 
  [coll] (map (fn [e] (key e)) coll))

(defn vals
  "Returns the values in the map" 
  [coll] (map (fn [e] (val e)) coll))

(defn assoc
  "Adds the keys and values to the map"
  ([map key val] (conj- (or map {}) (map-entry key val)))
  ([map key val & kvs]
    (let [new-map (conj- (or map {}) (map-entry key val))]
      (if kvs
        (if (next kvs)
          (recur new-map (first kvs) (second kvs) (nnext kvs))
          (throw "Value missing for key"))
        new-map))))

(defn assoc-in
  "Associates a value in nested data structure"
  [m [k & ks] v] (if ks
                   (assoc m k (assoc-in (get m k) ks v))
                   (assoc m k v)))

(defn get-in
  "Get a value in a nested data structure"
  ([m ks] (reduce get m ks)))

(def dissoc
  "Returns new map that does not contain the specified key. Very inefficient."
  (fn
    ([map] map)
    ([map k] (reduce conj- {} (filter (fn [e] (not= (key e) k)) map)))))

(defn update
  "Updates a value in map, where k is the key to be updates,
  f is a function that takes the old value and extra values starting from x,
  and returns the new value"
  ([m k f] (assoc m k (f (get m k))))
  ([m k f x] (assoc m k (f (get m k) x)))
  ([m k f x y] (assoc m k (f (get m k) x y)))
  ([m k f x y z] (assoc m k (f (get m k) x y z))))

(def into
  "Adds elements from collection from to collection to"
  (fn
    ([] [])
    ([to] to)
    ([to from] (reduce conj- to from))))

(defn frequencies
  "Calculates the counts of unique elements in coll and returns them in a map"
  [coll] (reduce (fn [counts x] (assoc counts x (inc (get counts x 0)))) {} coll))

; Printing and Reading

(defn slurp [fn] (apply str (clojure.lang.io/reader fn)))
(defn spit [f content] (let [prev-outport *out*
                             w (clojure.lang.io/writer f)
                             ]
                         (set! *out* w)
                         (print content)
                         (close w)
                         (set! *out* prev-outport)
                         nil))


(def printf (fn [& args] (print- (apply format args))))

(defn newline [] (print- \newline))
(defn print [& more] (run! print- more))
(defn println [& more] (apply print more) (newline))

(def *print-length* nil)

(defn pr
  "Prints values to *out* in a format understandable by the Reader"
  [& more] (run! (fn [x]
                   (cond (vector? x) (do (print "[")
                                         (when (not (empty? x))
                                           (if *print-length*
                                             (let [p (take *print-length* x)
                                                   r (drop *print-length* x)
                                                   ]
                                               (if (empty? p)
                                                 (print "...")
                                                 (do
                                                   (pr (first p))
                                                   (run! (fn [x] (print \space) (pr x)) (rest p))
                                                   (when (not (empty? r))
                                                     (print " ...")))))
                                             (do
                                               (pr (first x))
                                               (run! (fn [x] (print \space) (pr x)) (rest x)))))
                                         (print \]))
                         (map-entry? x) (do (print \[)
                                            (pr (key x))
                                            (print \space)
                                            (pr (val x))
                                            (print \]))
                         (set? x) (cond (empty? x) (print "#{}")
                                        (and *print-length* (<= *print-length* 0)) (print "#{...}")
                                        :else (do (print "#{")
                                                  (pr (first x))
                                                  (run! (fn [x] (print \space) (pr x)) (rest x))
                                                  (print \})))
                         (map? x) (cond (empty? x) (print "{}")
                                        (and *print-length* (<= *print-length* 0)) (print "{...}")
                                        :else (do (print \{)
                                                  (pr (key (first x)))
                                                  (print \space)
                                                  (pr (val (first x)))
                                                  (run! (fn [x] (print \space) (pr (key x))
                                                          (print \space) (pr (val x))) (rest x))
                                                  (print \})))
                         (seq? x) (cond (empty? x) (print "()")
                                        (and *print-length* (<= *print-length* 0)) (print "(...)")
                                        :else (do (print \()
                                                  (pr (first x))
                                                  (run! (fn [x] (print \space) (pr x)) (rest x))
                                                  (print \))))
                         (ratio? x) (do (color :green)
                                        (pr- (numerator x))
                                        (print \/)
                                        (pr- (denominator x))
                                        (color))
                         (or (nil? x) (boolean? x)) (do (color :blue)
                                                        (pr- x)
                                                        (color))
                         (char? x) (do (color :magenta)
                                       (pr- x)
                                       (color))
                         (number? x) (do (color :green)
                                         (pr- x)
                                         (color))
                         (string? x) (do (color :red)
                                         (pr- x)
                                         (color))
                         (keyword? x) (do (color :bold :yellow)
                                          (pr- x)
                                          (color))
                         :else (pr- x))
                   ) more))

(defn prn [& more] (apply pr more) (newline))

; Configure *print-hook* so that the REPL can print recursively
(def *print-hook* prn)

(def-macro (with-out-str body)
   `(let [ prev-outport *out*
           new-outport (clojure.lang.io/writer)
         ] (set! *out* new-outport)
	   ,body
           (set! *out* prev-outport)
           (java.lang.String new-outport)))

(def-macro (with-in-str s body)
  `(let ((prev-inport *in*)
         (new-inport (clojure.lang.io/reader (char-array ,s)))
         ) (set! *in* new-inport)
        (let ((r ,body))
          (set! *in* prev-inport)
          r
          )))

(defn pr-str [& xs] (with-out-str (apply pr xs)))
(defn prn-str [& xs] (with-out-str (apply prn xs)))
(defn print-str [& xs] (with-out-str (apply print xs)))
(defn println-str [& xs] (with-out-str (apply println xs)))

(def read-line (fn
                 ([] (read-line *in*))
                 ([rdr] (defn rls [acc]
                          (let [c (first rdr)]
                            (rest rdr)
                            (if (= c \newline)
                              (if (and (list? acc) (= (car acc) \return))
                                (cdr acc) acc)
                              (rls (cons c acc))))
                          )
                  (apply str (reverse (rls '()))))))

(defn read-string [s] (with-in-str s (read)))
(defn load-string [s] (eval (read-string s)))

(defn str [& args] (with-out-str (run! (fn [x] (cond (nil? x) nil
                                                     (char? x) (print x)
                                                     (string? x) (print x)
                                                     (number? x) (print x)
                                                     :else (pr x))) args)))

(defn line-seq [rdr] (let [line (read-line rdr)]
                       (if line
                         (cons line (lazy-seq (line-seq rdr))))))

; Vectors

(def subvec (fn ([v start] (drop start v))
     	        ([v start end] (take (- end start) (drop start v)))))
(defn rseq [x] (reverse (seq x)))

; Logic

(defn every? [pred coll] (cond (empty? coll) true
                               (pred (first coll)) (recur pred (rest coll))
                               :else false))

; Collections

(defn split-at
  "Splits coll into two parts at index n and returns the parts in a vector"
  [n coll] [(take n coll) (drop n coll)])

(defn not-empty [coll] (if (empty? coll) nil coll))

(defn distinct
  "Returns a lazy sequence with the elements of coll without duplicates"
  [coll] (let [f (fn [coll seen]
                   (cond (empty? coll) '()
                         (contains? seen (first coll)) (recur (rest coll) seen)
                         (cons (first coll) (lazy-seq (f (rest coll) (conj seen (first coll)))))
                         )
                   )]
           (f coll #{})))

(defn dedupe
  "Returns a lazy sequence with the consecutive duplicates removed"
  [coll] (if (empty? coll) '()
             (cons (first coll)
                   (let [f (fn [prior coll]
                             (cond (empty? coll) '()
                                   (= prior (first coll)) (f prior (rest coll))
                                   :else (cons (first coll)
                                               (lazy-seq (f (first coll) (rest coll))))))]
                     (f (first coll) (rest coll))))))

; Parsing and printin

(defn char-escape-string
  "Returns string representation for some control characters or nil"
  [c] (case c
        \newline "\\n"
        \tab  "\\t"
        \return "\\r"
        \" "\\\""
        \\  "\\\\"
        \formfeed "\\f"
        \backspace "\\b"))

(defn char-name-string
  "Returns the name of some control characters or nil"
  [c] (case c
        \newline "newline"
        \tab "tab"
        \space "space"
        \backspace "backspace"
        \formfeed "formfeed"
        \return "return"))

(defn parse-boolean
  "Parses a boolean"
  [s] (if (string? s)
        (case s
          "true" true
          "false" false
          nil)
        (throw "Not a string")))

; Functional

(defn partial
  "Creates a new function with the f called partially with the given arguments.
   The new function then takes the rest of the arguments"
  ([f] f)
  ([f arg1] (fn [& more-args] (apply f arg1 more-args)))
  ([f arg1 arg2] (fn [& more-args] (apply f arg1 arg2 more-args)))
  ([f arg1 arg2 arg3] (fn [& more-args] (apply f arg1 arg2 arg3 more-args)))
  ([f arg1 arg2 arg3 arg4] (fn [& more-args] (apply f arg1 arg2 arg3 arg4 more-args)))
  )

(defn complement
  "Returns a function that returns an opposite boolean value as the given function f"
  [f] (fn [& args] (not (apply f args))))

(defn comp
  "Returns a composition of the given functions"
  ([] identity)
  ([f] f)
  ([f g] (fn [& args] (f (apply g args))))
  ([f g h]) (fn [& args] (f (g (apply h args))))
  )

