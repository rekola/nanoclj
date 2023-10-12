; Utilities for types.

(defn vector? [x] (instance? clojure.lang.PersistentVector x))
(defn keyword? [x] (instance? clojure.lang.Keyword x))
(defn delay? [x] (instance? clojure.lang.Delay x))
(defn lazy-seq? [x] (instance? clojure.lang.LazySeq x))
(defn map-entry? [x] (instance? clojure.lang.MapEntry x))
(defn set? [x] (instance? clojure.lang.PersistentTreeSet x))
(defn map? [x] (instance? clojure.lang.PersistentArrayMap x))
(defn image? [x] (instance? nanoclj.core.Image x))
(defn inst? [x] (instance? java.util.Date x))
(defn uuid? [x] (instance? java.util.UUID x))
(defn coll? [x] (is-any-of? (type x)
                            clojure.lang.PersistentTreeSet
                            clojure.lang.PersistentArrayMap
                            clojure.lang.PersistentVector
                            clojure.lang.Cons
                            ))
(defn associative? [coll] (is-any-of? (type x)
                                      clojure.lang.PersistentVector
                                      clojure.lang.PersistentArrayMap))
(defn sequential?
  "Returns true if coll is sequential (ordered)"
  [coll] (is-any-of? (type coll) clojure.lang.PersistentVector clojure.lang.Cons))

(defn counted?
  "Returns true if coll implements count in constant time"
  [coll] (is-any-of? (type coll) clojure.lang.PersistentTreeSet clojure.lang.PersistentArrayMap clojure.lang.PersistentVector))

(defn seq?
  "Returns true if coll is a Sequence"
  [coll] (or (instance? clojure.lang.Cons coll) (instance? clojure.lang.LazySeq coll) (equals? (-bit-and (-get-cell-flags coll) 4) 4)))

(defn seqable?
  "Returns true if coll supportes seq"
  [coll] (is-any-of? (type coll) java.lang.String clojure.lang.LazySeq clojure.lang.PersistentVector clojure.lang.Cons clojure.lang.PersistentTreeSet clojure.lang.PersistentArrayMap))

(defn realized?
  "Returns true if Delay or LazySeq has been realized"
  [x] (equals? (-bit-and (-get-cell-flags x) 8) 8))

(def keyword clojure.lang.Keyword)
(def char nanoclj.core.Codepoint)
(def short java.lang.Integer)
(def double java.lang.Double)
(def float java.lang.Double)

(def resolve ns-resolve)

; Casts argument to int
(def int java.lang.Integer)

; Casts argument to long (or int if sufficient)
(def long java.lang.Long)

(defn inst-ms
  "Returns millisecond time of java.util.Date"
  [x] (long x))

(defn num
  "Casts argument to number"
  [x] (if (number? x) x (long x)))

(defn vec [coll] (reduce -conj [] coll))
(defn set [coll] (reduce -conj #() coll))

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
  [n] (if (ratio? n) (second n) 1))

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

(def subs (fn ([s start]     (apply str (drop start s)))
     	      ([s start end] (apply str (take (- end start) (drop start s))))))


; Sequences

(defn filter
  "Returns a sequence with the elements from coll for which the pred returns true"
  [pred coll] (cond (empty? coll) '()
        (pred (first coll)) (cons (first coll) (filter pred (rest coll)))
        :else (filter pred (rest coll))))

(defn keep
  "Returns a lazy sequence of the non-nil results of (f item)"
  [f coll] (cond (empty? coll) '()
                 (let [ v (f (first coll)) ]
                   (if (nil? v)
                     (keep f (rest coll))
                     (cons v (lazy-seq (keep f (rest coll))))))))

(defn remove [pred coll]
  (cond (empty? coll) '()
        (pred (first coll)) (remove pred (rest coll))
        :else (cons (first coll) (remove pred (rest coll)))))

(def concat (fn
              ([] '())
              ([x] x)
              ([x y] (if (empty? x) y (cons (first x) (concat (rest x) y))))))

(def iterate (fn [f x] (cons x (lazy-seq (iterate f (f x))))))

(def repeat (fn ([x]   (cons x (lazy-seq (repeat x))))
                ([n x] (if (<= n 0) '() (cons x (repeat (dec n) x))))))

(defn last
  "Returns the last element of sequence"
  [[f & r]] (if r (recur r) f))

(def butlast (fn [coll] (if (next coll)
                          (cons (first coll) (recur (next coll)))
                          '())))

(defn reverse [coll] (reduce -conj '() coll))

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

; Vectors

(def mapv
  "Returns a vector with each element mapped using f"
  (fn
    ([f coll] (loop [ coll coll acc [] ] (if (empty? coll) acc (recur (rest coll) (conj acc (f (first coll)))))))
    ([f c1 c2] (loop [ c1 c1 c2 c2 acc [] ] (if (or (empty? c1) (empty? c2)) acc (recur (rest c1) (rest c2) (conj acc (f (first c1) (first c2)))))))))

(defn filterv
  "Returns a vector with the elements from coll for which the pred returns true"
  [pred coll] (loop [coll coll acc []] (cond (empty? coll) acc
                                             (pred (first coll)) (recur (rest coll) (conj acc (first coll)))
                                             :else (recur (rest coll) acc))))

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

(defn take [n coll] (if (or (le n 0) (empty? coll)) '() (cons (first coll) (lazy-seq (take (dec n) (rest coll))))))

(defn drop [n coll] (if (le n 0) coll (recur (dec n) (rest coll))))

(def map
  "Returns a lazy sequence with each element mapped using f"
  (fn
    ([f coll] (if (empty? coll) '() (cons (f (first coll)) (lazy-seq (map f (rest coll))))))
    ([f c1 c2] (if (or (empty? c1) (empty? c2)) '() (cons (f (first c1) (first c2)) (lazy-seq (map f (rest c1) (rest c2))))))))

(def repeatedly (fn ([f]   (cons (f) (lazy-seq (repeatedly f))))
                    ([n f] (if (<= n 0) '() (cons (f) (lazy-seq (repeatedly (dec n) f)))))))

; Maps & Sets

(def key first)
(def val second)

(defn keys
  "Returns the values in the map" 
  [coll] (map (fn [e] (key e)) coll))

(defn vals
  "Returns the values in the map" 
  [coll] (map (fn [e] (val e)) coll))

(defn assoc
  "Adds the keys and values to the map"
  ([map key val] (-conj (or map {}) (map-entry key val)))
  ([map key val & kvs]
    (let [new-map (-conj (or map {}) (map-entry key val))]
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
    ([map k] (reduce -conj {} (filter (fn [e] (not= (key e) k)) map)))))

(defn disj
  "Removes keys from a set"
  ([set] set)
  ([set key] (-disj set key)))

(defn merge
  "Merges multiple maps"
  [& maps] (reduce #( conj (or %1 {}) %2 ) maps))

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
    ([to from] (reduce -conj to from))))

(defn frequencies
  "Calculates the counts of unique elements in coll and returns them in a map"
  [coll] (reduce (fn [counts x] (assoc counts x (inc (get counts x 0)))) {} coll))

; Printing and Reading

(defn slurp
  "Reads a file"
  [fn] (let [rdr (clojure.java.io/reader fn)
             f (fn [s] (let [c (.read rdr)] (if (= c ##Eof) s (recur (conj s c)))))
             r (f "")]
         (.close rdr)
         r))

(defn spit
  "Writes content to a file"
  [f content] (let [prev-out *out*
                    w (clojure.java.io/writer f)
                    ]
                (set! *out* w)
                (print (str content))
                (.close w)
                (set! *out* prev-out)
                nil))

(def printf (fn [& args] (-print (apply format args))))

(defn newline [] (-print \newline))

(defn warn [& more] (let [prev-out *out*]                          
                      (set! *out* *err*)
	              (apply println more)
                      (set! *out* prev-out)
                      nil))

(def *print-length* nil)

(def -styles {
              :scalar [ 0.5 0.62 0.5 ]
              :boolean [ 0.86 0.55 0.76 ]
              :nil [ 0.86 0.55 0.76 ]
              :char [ 0.8 0.58 0.58 ]
              :string [ 0.8 0.58 0.58 ]
              :symbol [ 0.94 0.87 0.69 ]
              :keyword [ 0.94 0.87 0.69 ]
              })

  
(defn pr-generic
  "Prints values to *out* in a format understandable by the Reader"
  [print-fn & more] (run! (fn [x]
                            (cond (map-entry? x) (do (-print \[)
                                                     (pr-generic print-fn (key x))
                                                     (print \space)
                                                     (pr-generic print-fn (val x))
                                                     (-print \]))
                                  (seq? x) (cond (empty? x) (-print "()")
                                                 (and *print-length* (<= *print-length* 0)) (-print "(...)")
                                                 :else (do (-print \()
                                                           (pr-generic print-fn (first x))
                                                           (run! (fn [x] (-print \space) (pr-generic print-fn x)) (rest x))
                                                           (-print \))))
                                  (vector? x) (do (-print "[")
                                                  (when (seq x)
                                                             (if *print-length*
                                                               (let [p (take *print-length* x)
                                                                     r (drop *print-length* x)
                                                                     ]
                                                                 (if (empty? p)
                                                                   (-print "...")
                                                           (do
                                                             (pr-generic print-fn (first p))
                                                             (run! (fn [x] (-print \space) (pr-generic print-fn x)) (rest p))
                                                             (when (seq r)
                                                               (-print " ...")))))
                                                       (do
                                                         (pr-generic print-fn (first x))
                                                         (run! (fn [x] (-print \space) (pr-generic print-fn x)) (rest x)))))
                                                   (print \]))
                                   (set? x) (cond (empty? x) (-print "#{}")
                                                  (and *print-length* (<= *print-length* 0)) (-print "#{...}")
                                                  :else (do (-print "#{")
                                                            (pr-generic print-fn (first x))
                                                            (run! (fn [x] (-print \space) (pr-generic print-fn x)) (rest x))
                                                            (-print \})))
                                   (map? x) (cond (empty? x) (-print "{}")
                                                  (and *print-length* (<= *print-length* 0)) (-print "{...}")
                                                  :else (do (-print \{)
                                                            (pr-generic print-fn (key (first x)))
                                                            (-print \space)
                                                            (pr (val (first x)))
                                                            (run! (fn [x] (-print ", ") (pr-generic print-fn (key x))
                                                                    (-print \space) (pr-generic print-fn (val x))) (rest x))
                                                            (-print \})))                
                                   (ratio? x) (print-fn :scalar x)
                                   (nil? x) (print-fn :nil x)
                                   (boolean? x) (print-fn :boolean x)
                                   (char? x) (print-fn :char x)
                                   (number? x) (print-fn :scalar x)
                                   (string? x) (print-fn :string x)
                                   (keyword? x) (print-fn :keyword x)
                                   (symbol? x) (print-fn :symbol x)
                                   (or (closure? x) (macro? x)) (pr-generic print-fn (cons 'fn (car x)))
                                   (image? x) (let [ws *window-size*
                                                    f *window-scale-factor*
                                                    w (x :width)
                                                    h (x :height)
                                                    ww (* (ws 0) f)
                                                    wh (* (ws 1) f)
                                                    s (/ ww w )
                                                    ]
                                                (if (> w wh)
                                                  (-pr (Image/resize x ww (* s h)))
                                                  (-pr x)
                                                  ))
                                   :else (print-fn nil x))
                            ) more))

(defn print
  "Prints args into *out* for human reading"
  [& more] (apply pr-generic (fn [class v] (-print v)) more))

(defn println
  "Prints args into *out* using print followed by a newline"
  [& more] (apply print more) (newline))

(defn pr
  "Prints args into *out* for Reader with color"
  [& more] (apply pr-generic (fn [class v]
                               (let [color (get -styles class)]
                                 (if color
                                   (do
                                     (save)
                                     (set-color color)
                                     (-pr v)
                                     (restore))
                                   (-pr v)))) more))

(defn prn
  "Prints args into *out* using pr followed by a newline"
  [& more] (apply pr more) (newline))

; Configure *print-hook* so that the REPL can print recursively
(def *print-hook* prn)

(def-macro (with-out-str body)
   `(let [ prev-out *out*
           w (clojure.java.io/writer)
         ] (set! *out* w)
	   ,body
           (set! *out* prev-out)
           (java.lang.String w)))

(def-macro (with-in-str s body)
  `(let ((prev-in *in*)
         (rdr (clojure.java.io/reader (char-array ,s)))
         )
     (set! *in* rdr)
     (let ((r ,body))
       (set! *in* prev-in)
       r
       )))

(def-macro (with-out new-out body)
  `(let ((prev-out *out*)
         (tmp ,new-out))
     (set! *out* tmp)
     ,body
     (set! *out* prev-out)
     tmp))

(defn pr-str [& xs] (with-out-str (apply pr xs)))
(defn prn-str [& xs] (with-out-str (apply prn xs)))
(defn print-str [& xs] (with-out-str (apply print xs)))
(defn println-str [& xs] (with-out-str (apply println xs)))

(defn read-string [s] (with-in-str s (read)))
(defn load-string [s] (eval (read-string s)))

(defn str [& args] (with-out-str (apply print args)))

; Logic

(defn every?
  "Returns true if pred is true for every item in the collection"
  [pred coll] (cond (empty? coll) true
                    (pred (first coll)) (recur pred (rest coll))
                    :else false))

(defn not-every?
  "Returns false if pred is true for every item in the collection"
  [pred coll] (cond (empty? coll) false
                    (pred (first coll)) (recur pred (rest coll))
                    :else true))

(defn not-any?
  "Returns false if pred is true for any item in the collection"
  [pred coll] (cond (empty? coll) true
                    (pred (first coll)) false
                    :else (recur pred (rest coll))))

; Collections

(defn nthrest
  "Returns the nth rest of coll"
  [coll n] (cond (< n 0) (throw "Invalid argument")
                 (= n 0) coll
                 :else (recur (rest coll) (dec n))))

(defn nthnext
  "Returns the nth next of coll"
  [coll n] (cond (< n 0) (throw "Invalid argument")
                 (= n 0) (seq coll)
                 :else (recur (next coll) (dec n))))

(defn cycle
  "Returns a repeating, lazy, infinite sequence of the elements in coll"
  [coll] (if (empty? coll) '()
             (let [f (fn [remaining all]
                       (if remaining
                         (cons (first remaining) (lazy-seq (f (next remaining) all)))
                         (cons (first all) (lazy-seq (f (next all) all)))))]
               (f nil coll))))
                             
(defn flatten
  "Breaks any nested structure in the coll and returns the content in a flat sequence."  
  [coll] (cond (empty? coll) '()
               (coll? (first coll)) (concat (flatten (first coll)) (flatten (rest coll)))
               :else (cons (first coll) (flatten (rest coll)))))
                                                                               
(defn partition
  "Partitions the collection into n sized blocks"
  [n coll] (if (empty? coll)
             '()
             (cons (take n coll) (lazy-seq (partition n (drop n coll))))))

(defn split-at
  "Splits coll into two parts at index n and returns the parts in a vector"
  [n coll] [(take n coll) (drop n coll)])

(defn not-empty [coll] (if (empty? coll) nil coll))

(defn distinct
  "Returns a lazy sequence with the elements of coll without duplicates"
  [coll] (let [f (fn [coll seen]
                   (cond (empty? coll) '()
                         (contains? seen (first coll)) (recur (rest coll) seen)
                         (cons (first coll) (lazy-seq (f (rest coll) (-conj seen (first coll)))))
                         )
                   )]
           (f coll #{})))

(defn distinct?
  "Returns true if none of the arguments are equal to any other"
  ([x] true)
  ([x y] (not= x y))
  ([x y & more] (if (= x y) false
                    (let [f (fn [s coll]
                              (cond (empty? coll) true
                                    (contains? s (first coll)) false
                                    :else (recur (conj s (first coll)) (rest coll))
                                    ))]
                      (f #{ x y } more)))))
                     
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

(defn clojure-version
  "Returns the nanoclj version string"
  [] (System/getProperty "nanoclj.version"))

(def isa? instance?)

(defn parents
  "Returns a set with the immmediate parents of the provided object"
  [t] (conj #{} (rest t)))
