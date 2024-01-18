; Utilities for types.

(defn vector? [x] (instance? clojure.lang.PersistentVector x))
(defn keyword? [x] (instance? clojure.lang.Keyword x))
(defn delay? [x] (instance? clojure.lang.Delay x))
(defn lazy-seq? [x] (instance? clojure.lang.LazySeq x))
(defn map-entry? [x] (instance? clojure.lang.MapEntry x))
(defn set? [x] (instance? clojure.lang.APersistentSet x))
(defn map? [x] (instance? clojure.lang.APersistentMap x))
(defn list-map? [x] (instance? nanoclj.core.ListMap x))
(defn image? [x] (instance? nanoclj.core.Image x))
(defn gradient? [x] (instance? nanoclj.core.Gradient x))
(defn inst? [x] (instance? java.util.Date x))
(defn uuid? [x] (instance? java.util.UUID x))
(defn coll? [x] (or (identical? x '())
                    (instance? clojure.lang.APersistentSet x)
                    (instance? clojure.lang.APersistentMap x)
                    (is-any-of? (type x)
                                clojure.lang.PersistentVector
                                clojure.lang.PersistentQueue
                                clojure.lang.Cons
                            )))
(defn associative? [coll] (or (instance? clojure.lang.PersistentVector x)
                              (instance? clojure.lang.APersistentMap x)))
(defn sequential?
  "Returns true if coll is sequential (ordered)"
  [coll] (is-any-of? (type coll) clojure.lang.PersistentVector clojure.lang.Cons))

(defn indexed?
  [coll] (instance? clojure.lang.PersistentVector coll))

(defn counted?
  "Returns true if coll implements count in constant time"
  [coll] (or (instance? clojure.lang.APersistentSet coll)
             (instance? clojure.lang.APersistentMap coll)
             (instance? clojure.lang.PersistentVector coll)))

(defn seq?
  "Returns true if coll is a Sequence"
  [coll] (or (instance? clojure.lang.Cons coll) (instance? clojure.lang.LazySeq coll) (equals? (-bit-and (-get-cell-flags coll) 1024) 1024)))

(defn seqable?
  "Returns true if coll supports seq"
  [coll] (or (instance? java.lang.String coll)
             (instance? clojure.lang.LazySeq coll)
             (instance? clojure.lang.PersistentVector coll)
             (instance? clojure.lang.Cons coll)
             (instance? clojure.lang.APersistentSet coll)
             (instance? clojure.lang.APersistentMap coll)))

(defn realized?
  "Returns true if Delay or LazySeq has been realized"
  [x] (equals? (-bit-and (-get-cell-flags x) 2048) 2048))

(def keyword clojure.lang.Keyword)
(def char nanoclj.core.Codepoint)
(def double java.lang.Double)
(def float java.lang.Double)
(def byte java.lang.Byte)
(def short java.lang.Short)
(def int java.lang.Integer)
(def long java.lang.Long)
(def bigint clojure.lang.BigInt)
(def biginteger clojure.lang.BigInt)

(defn inst-ms
  "Returns millisecond time of java.util.Date"
  [x] (long x))

(defn vec [coll] (reduce -conj [] coll))
(defn set [coll] (reduce -conj #() coll))

(defn run! [proc coll] (if (empty? coll) nil (do (proc (first coll)) (run! proc (rest coll)))))

(def image nanoclj.core.Image)

; Utilities for math.

(defn even?
  "Returns true if the argument is even"
  [n] (zero? (rem n 2)))

(defn odd?
  "Returns true if the argument is odd"
  [n] (not (even? n)))

(defn pos? [n] (> n 0))
(defn neg? [n] (< n 0))
(defn pos-int? [n] (and (int? n) (> n 0)))
(defn neg-int? [n] (and (int? n) (< n 0)))
(defn nat-int? [n] (and (int? n) (>= n 0)))
(defn float? [n] (instance? java.lang.Double x))
(defn double? [n] (instance? java.lang.Double x))
(defn NaN? [n] (identical? n ##NaN))
(defn infinite? [n] (or (identical? n ##Inf) (identical? n ##-Inf)))

(defn mod
  "Modulus of num and div. Not the same as C's % operator (which is rem) but Knuth's mod (truncating towards negativity)."
  [num div] (let [m (rem num div)]
              (if (or (zero? m) (= (pos? num) (pos? div)))
                m
                (+ m div))))

(defn ratio?
  "Returns true if n is a Ratio"
  [n] (instance? clojure.lang.Ratio n))

(defn rational?
  "Returns true if n is an integer or a Ratio"
  [n] (or (integer? n) (instance? clojure.lang.Ratio n)))

(defn numerator
  "Returns the numerator of a Ratio"
  [n] (if (ratio? n) (n 0) n))

(defn denominator
  "Returns the denominator of a Ratio"
  [n] (if (ratio? n) (n 1) 1))

; Miscellaneous

(defn ns-interns [ns] (doall (map first (reduce concat '() (car ns)))))

(defn new [t & args]
  "Creates an object"
  (apply* t args))

(defn identity [x] x)

(def some (fn [pred coll] (cond (empty? coll) false
                                (pred (first coll)) true
				:else (some pred (rest coll)))))
(def-macro (time body)
   `(let [ t0 (System/currentTimeMillis) ]
        (let ((e ~body))
	     (println (str "Elapsed time: " (- (System/currentTimeMillis) t0) " msecs"))
	     e)))

(def-macro (comment body) nil)

(defn constantly
  "Creates a function that always returns x and accepts any number of arguments"
  [x] (fn [& args] x))

(def hash-ordered-coll hash)
(def hash-unordered-coll hash)

(defn juxt
  ([& fs] (fn [& xs] (reduce #(conj %1 (apply %2 xs)) [] fs))))

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

(defn take-last
  "Take last n elements from coll"
  [n coll] (let [nd (- (count coll) n)] (if (> nd 0) (drop nd coll) coll)))

(defn drop-last
  "Drop last n elements from coll"
  ([coll] (if (next coll) (cons (first coll) (lazy-seq (drop-last (next coll)))) '()))
  ([n coll] (if (gt (bounded-count (inc n) coll) n) (cons (first coll) (lazy-seq (drop-last n (next coll)))) '())))

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

(defn hash-set
  "Creates a hash set from the arguments"
  ([] (clojure.lang.PersistentHashSet))
  ([& keys] (reduce -conj (clojure.lang.PersistentHashSet) keys)))

(defn sorted-set
  "Creates a sorted hash set from the arguments"
  ([] (clojure.lang.PersistentTreeSet))
  ([& keys] (reduce -conj (clojure.lang.PersistentTreeSet) keys)))

(defn array-map
  "Creates an array-map from the arguments"
  ([] (clojure.lang.PersistentArrayMap))
  ([& keyvals] (let [f (fn [val seq]
                         (if (empty? seq)
                           val
                           (recur (-conj val
                                         (map-entry
                                          (first seq)
                                          (first (rest seq))))
                                  (rest (rest seq))
                                  )))]
                 (f (clojure.lang.PersistentArrayMap) keyvals))))

(defn hash-map
  "Creates an array-map from the arguments"
  ([] (clojure.lang.PersistentHashMap))
  ([& keyvals] (let [f (fn [val seq]
                         (if (empty? seq)
                           val
                           (recur (-conj val
                                         (map-entry
                                          (first seq)
                                          (first (rest seq))))
                                  (rest (rest seq))
                                  )))]
                 (f (clojure.lang.PersistentHashMap) keyvals))))

(defn sorted-map
  "Creates a sorted map from the arguments"
  ([] (clojure.lang.PersistentTreeMap))
  ([& keyvals] (let [f (fn [val seq]
                         (if (empty? seq)
                           val
                           (recur (-conj val
                                         (map-entry
                                          (first seq)
                                          (first (rest seq))))
                                  (rest (rest seq))
                                  )))]
                 (f (clojure.lang.PersistentTreeMap) keyvals))))

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
          (throw (new RuntimeException "Value missing for key")))
        new-map))))

(defn assoc-in
  "Associates a value in nested data structure"
  [m [k & ks] v] (if ks
                   (assoc m k (assoc-in (get m k) ks v))
                   (assoc m k v)))

(defn get-in
  "Get a value in a nested data structure"
  ([m ks] (reduce get m ks))
  ([m ks not-found] (loop [ ks ks m m ]
                      (if (empty? ks)
                        m
                        (if (contains? m (first ks))
                          (recur (rest ks) (m (first ks)))
                          not-found)))))

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

(defn group-by
  "Groups elements in coll based on the function f"
  [f coll] (loop [coll coll m {}]
             (if (empty? coll)
               m
               (recur (rest coll) (update m (f (first coll)) (fn [v] (conj (or v []) (first coll))))))))

(defn list-map
  "Creates a list-map from the arguments"
  [& keyvals] (apply* nanoclj.core.ListMap keyvals))

(defn zipmap
  "Returns a map with the keys and vals"
  [keys vals] (if (or (empty? keys) (empty? vals))
                {}
                (assoc (zipmap (rest keys) (rest vals)) (first keys) (first vals))))

; Printing and Reading

(defn slurp
  "Reads a file. Arguments are passed to Reader so same kind of input is supported
  (e.g. URL or filename)"
  [f & opts] (let [rdr (apply clojure.java.io/reader f opts)
                   fun (fn [s] (let [c (-read rdr)] (if (= c ##Eof) s (recur (conj s c)))))
                   r (fun "")]
               (-close rdr)
               r))

(defn read-line
  "Reads a line from *in* or a reader"
  ([] (read-line *in*))
  ([rdr] (let [c0 (-read rdr)]
           (if (= c0 ##Eof) nil
               (loop [c c0 acc ""]
                 (if (or (= c ##Eof) (= c \newline)) acc (recur (-read rdr) (conj acc c))))))))

(defn line-seq
  "Returns a sequence of line from rdr"
  [rdr] (let [line (read-line rdr)]
          (if line
            (cons line (lazy-seq (line-seq rdr))))))

(defn spit
  "Writes content to a file. Arguments are passed to Writer so same kind of input is supported"
  [f content & options] (let [prev-out *out*
                              w (apply clojure.java.io/writer f options)
                              ]
                          (set! *out* w)
                          (print (str content))
                          (-close w)
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
(def *print-meta* false)

(def ^:private print-styles {:scalar [ 0.5 0.62 0.5 ]
                             :boolean [ 0.86 0.55 0.76 ]
                             :nil [ 0.86 0.55 0.76 ]
                             :char [ 0.8 0.58 0.58 ]
                             :string [ 0.8 0.58 0.58 ]
                             :symbol [ 0.94 0.87 0.69 ]
                             :keyword [ 0.94 0.87 0.69 ]
                             :var [ 0.94 0.87 0.69 ]
                             :class [ 0.55 0.82 0.83 ]
                             })

(defn ^:private pr-inline
  "Prints x to *out* in a format understandable by the Reader"
  [print-fn x] (cond (map-entry? x) (do (-print \[)
                                        (pr-inline print-fn (key x))
                                        (print \space)
                                        (pr-inline print-fn (val x))
                                        (-print \]))
                     (seq? x) (cond (empty? x) (-print "()")
                                    (and *print-length* (<= *print-length* 0)) (-print "(...)")
                                    :else (do (-print \()
                                              (pr-inline print-fn (first x))
                                              (run! (fn [x] (-print \space) (pr-inline print-fn x)) (rest x))
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
                                               (pr-inline print-fn (first p))
                                               (run! (fn [x] (-print \space) (pr-inline print-fn x)) (rest p))
                                               (when (seq r)
                                                 (-print " ...")))))
                                         (do
                                           (pr-inline print-fn (first x))
                                           (run! (fn [x] (-print \space) (pr-inline print-fn x)) (rest x)))))
                                     (print \]))
                     (set? x) (cond (empty? x) (-print "#{}")
                                    (and *print-length* (<= *print-length* 0)) (-print "#{...}")
                                    :else (do (-print "#{")
                                              (pr-inline print-fn (first x))
                                              (run! (fn [x] (-print \space) (pr-inline print-fn x)) (rest x))
                                              (-print \})))
                     (list-map? x) (do (-print \{)
                                       (pr-inline print-fn (key x))
                                       (-print \space)
                                       (pr-inline print-fn (val x))
                                       (run! (fn [x]
                                               (-print ", ")
                                               (pr-inline print-fn (key x))
                                               (-print \space)
                                               (pr-inline print-fn (val x))) (rest x))
                                       (-print \}))
                     (map? x) (cond (empty? x) (-print "{}")
                                    (and *print-length* (<= *print-length* 0)) (-print "{...}")
                                    :else (do (-print \{)
                                              (pr-inline print-fn (key (first x)))
                                              (-print \space)
                                              (pr-inline print-fn (val (first x)))
                                              (run! (fn [x] (-print ", ") (pr-inline print-fn (key x))
                                                      (-print \space) (pr-inline print-fn (val x))) (rest x))
                                              (-print \})))
                     (ratio? x) (print-fn :scalar x)
                     (nil? x) (print-fn :nil x)
                     (boolean? x) (print-fn :boolean x)
                     (char? x) (print-fn :char x)
                     (number? x) (print-fn :scalar x)
                     (string? x) (print-fn :string x)
                     (keyword? x) (print-fn :keyword x)
                     (symbol? x) (print-fn :symbol x)
                     (var? x) (print-fn :var x)
                     (class? x) (print-fn :class x)
                     (closure? x) (pr-inline print-fn (cons 'fn (car x)))
                     (image? x) (let [cs *cell-size*
                                      f *window-scale-factor*
                                      w (x :width)
                                      h (x :height)
                                      ww (* (cs 0) f)
                                      wh (* (cs 1) f)
                                      s (/ wh h )
                                      ]
                                  (if (> w wh)
                                    (do
                                      (mode :inline)
                                      (-pr (Image/resize x (* s w) wh)))
                                    (-pr x)
                                    ))
                     (gradient? x) (do
                                     (mode :inline)
                                     (-pr (nanoclj.art/plot-gradient x)))
                     :else (print-fn nil x)))

(defn ^:private pr-meta
  [print-fn x sep] (when *print-meta*
                     (let [m (meta x)]
                       (when m (print \^) (pr-inline print-fn m) (print sep)))))

(defn ^:private pr-block
  [print-fn x]
  (if (image? x) (let [ws *window-size*
                       f *window-scale-factor*
                       w (x :width)
                       h (x :height)
                       ww (* (ws 0) f)
                       wh (* (ws 1) f)
                       s (/ ww w )
                       ]
                   (pr-meta print-fn x \newline)
                   (if (> w wh)
                     (do
                       (mode :block)
                       (-pr (Image/resize x ww (* s h))))
                     (-pr x)
                     ))
      (do (pr-meta print-fn x \space)
          (pr-inline print-fn x))))

(defn print
  "Prints args into *out* for human reading"
  [& more] (run! #( pr-block (fn [class v] (-print v)) %1 ) more))

(defn println
  "Prints args into *out* using print followed by a newline"
  [& more] (apply print more) (newline))

(defn ^:private pr-with-class
  [class v] (let [color (get print-styles class)]
              (if color
                (do
                  (save)
                  (set-color color)
                  (-pr v)
                  (restore))
                (-pr v))))
(defn pr
  "Prints args into *out* for Reader with color"
  ([] nil)
  ([x] (pr-block pr-with-class x))
  ([x & more] (let [sep (if (image? x) "" " ")]
                (pr-block pr-with-class x)
                (run! (fn [x] (-print sep) (pr-block pr-with-class x)) more))))

(defn prn
  "Prints args into *out* using pr followed by a newline"
  [& more] (apply pr more) (newline))

(def-macro (with-out new-out & body)
  `(let ((prev-out *out*)
         (tmp ~new-out))
     (set! *out* tmp)
     ~@body
     (set! *out* prev-out)
     tmp))

(def-macro (with-out-pdf filename width height & body)
  `(let ((prev-out *out*)
         (w (clojure.java.io/writer ~width ~height :pdf ~filename)))
     (set! *out* w)
     ~@body
     (set! *out* prev-out)
     (-close w)
     nil))

(def-macro (with-out-str body)
   `(let [ prev-out *out*
           w (clojure.java.io/writer)
         ] (set! *out* w)
	   ~body
           (set! *out* prev-out)
           (java.lang.String w)))

(def-macro (with-in-str s & body)
  `(let ((prev-in *in*)
         (rdr (clojure.java.io/reader (char-array ~s)))
         )
     (set! *in* rdr)
     (let ((r ~@body))
       (set! *in* prev-in)
       r
       )))

(defn pr-str [& xs] (with-out-str (apply pr xs)))
(defn prn-str [& xs] (with-out-str (apply prn xs)))
(defn print-str [& xs] (with-out-str (apply print xs)))
(defn println-str [& xs] (with-out-str (apply println xs)))

(defn read-string [s] (with-in-str s (read)))
(defn load-string [s] (eval (read-string s)))

(defn ^:private str-print
  [& more] (run! #( if (coll? %) (pr-block (fn [class v] (-pr v)) %) (-str %) ) more))

(defn str
  "Converts arguments to string"
  ([] "")
  ([x] (with-out-str (str-print x)))
  ([x & ys] (with-out-str (do (str-print x)
                              (run! #( str-print % ) ys)))))

(def maps
  "Returns a string with each element mapped using f"
  (fn
    ([f coll] (loop [ coll coll acc "" ] (if (empty? coll) acc (recur (rest coll) (conj acc (f (first coll)))))))
    ([f c1 c2] (loop [ c1 c1 c2 c2 acc "" ] (if (or (empty? c1) (empty? c2)) acc (recur (rest c1) (rest c2) (conj acc (f (first c1) (first c2)))))))))

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

(defn true? [x] (equals? x true))
(defn false? [x] (equals? x false))

; Collections

(def nth
  "Returns the nth element of coll"
  (fn
    ([coll index]
     (cond (vector? coll) (coll index)
     	   (string? coll) (coll index)
     	   (or (< index 0) (empty? coll)) (throw (new IndexOutOfBoundsException "Index out of bounds"))
           (if (zero? index) (first coll) (recur (next coll) (dec index)))))
    ([coll index not-found]
     (cond (vector? coll) (coll index not-found)
     	   (string? coll) (coll index not-found)
     	   (or (< index 0) (empty? coll)) not-found
     	   :else (if (zero? index) (first coll) (recur (next coll) (dec index) not-found))))))

(defn third [coll] (first (next (next coll))))

(defn nthrest
  "Returns the nth rest of coll"
  [coll n] (cond (< n 0) (throw (new RuntimeException "Invalid argument"))
                 (= n 0) coll
                 :else (recur (rest coll) (dec n))))

(defn nthnext
  "Returns the nth next of coll"
  [coll n] (cond (< n 0) (throw (new RuntimeException "Invalid argument"))
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

; Parsing and printing

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
        (throw (new RuntimeException "Not a string"))))

(defn parse-long
  "Parses a long"
  [s] (try (long s)
           (catch NumberFormatException e nil)))

(defn parse-double
  "Parses a double"
  [s] (try (double s)
           (catch NumberFormatException e nil)))

(defn parse-uuid
  "Parses an UUID"
  [s] (java.util.UUID s))

(defn random-uuid
  "Creates a random UUID"
  [] (java.util.UUID/randomUUID))

; Functional

(defn partial
  "Creates a new function with the f called partially with the given arguments.
   The new function then takes the rest of the arguments"
  ([f] f)
  ([f arg1] (fn [& more-args] (apply f arg1 more-args)))
  ([f arg1 arg2] (fn [& more-args] (apply f arg1 arg2 more-args)))
  ([f arg1 arg2 arg3] (fn [& more-args] (apply f arg1 arg2 arg3 more-args)))
  ([f arg1 arg2 arg3 & more] (fn [& more-args] (apply f arg1 arg2 arg3 (concat more more-args)))))

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

(defn cast
  "Returns x if it is an instance of class c, otherwise throws an exception"
  [c x] (if (isa? c x) x (throw (new RuntimeException "Wrong class"))))

(defn num
  "Returns x if it is a number, otherwise throws an exception"
  [x] (if (isa? java.lang.Number x) x (throw (new RuntimeException "Not a number"))))

(defn parents
  "Returns a set with the immmediate parents of the provided object"
  [t] (conj #{} (rest t)))

(defn bounded-count
  "Returns the count if coll is counted? or counts the elements up to n"
  [n coll] (if (counted? coll)
             (count coll)
             (loop [c 0 n n coll coll] (if (or (empty? coll) (<= n 0)) c (recur (inc c) (dec n) (rest coll))))))

; Random numbers

(defn rand
  "Returns a random double in [0, n[. By default n is 1."
  ([] (Math/random))
  ([n] (* n (Math/random))))

(defn rand-int
  "Returns a random integer in [0, n["
  [n] (int (* n (Math/random))))

(defn rand-nth [coll] (nth coll (rand-int (count coll))))

; Arrays & tensors

(def tensor nanoclj.core.Tensor)

(defn boolean-array
  "Creates a boolean array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (boolean-array (count size-or-seq) size-or-seq)
                   (boolean-array size-or-seq false)))
  ([size init-val-or-seq] (tensor java.lang.Boolean/TYPE init-val-or-seq size)))

(defn byte-array
  "Creates a byte array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (byte-array (count size-or-seq) size-or-seq)
                   (byte-array size-or-seq 0)))
  ([size init-val-or-seq] (tensor java.lang.Byte/TYPE init-val-or-seq size)))

(defn char-array
  "Creates an array of UTF-8 code units from a string or from size and initial value or sequence. The resulting array is a byte array."
  [str] (tensor str))

(defn short-array
  "Creates a short array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (short-array (count size-or-seq) size-or-seq)
                   (short-array size-or-seq 0)))
  ([size init-val-or-seq] (tensor java.lang.Short/TYPE init-val-or-seq size)))

(defn int-array
  "Creates an int array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (int-array (count size-or-seq) size-or-seq)
                   (int-array size-or-seq 0)))
  ([size init-val-or-seq] (tensor java.lang.Integer/TYPE init-val-or-seq size)))

(defn float-array
  "Creates a float array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (float-array (count size-or-seq) size-or-seq)
                   (float-array size-or-seq 0.0)))
  ([size init-val-or-seq] (tensor java.lang.Float/TYPE init-val-or-seq size)))

(defn double-array
  "Creates a float array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (double-array (count size-or-seq) size-or-seq)
                   (double-array size-or-seq 0.0)))
  ([size init-val-or-seq] (tensor java.lang.Double/TYPE init-val-or-seq size)))

(defn object-array
  "Creates a object array of specified size"
  ([size-or-seq] (if (seqable? size-or-seq)
                   (object-array (count size-or-seq) size-or-seq)
                   (object-array size-or-seq nil)))
  ([size init-val-or-seq] (tensor java.lang.Object/TYPE init-val-or-seq size)))

(defn to-array
  "Creates an Object array from coll"
  [coll] (tensor java.lang.Object/TYPE coll (count coll)))

(defn into-array
  "Creates an array from aseq. If type is not provided, it's taken from the first element of aseq"
  ([aseq] (into-array (class (first aseq)) aseq))
  ([type aseq] (tensor (if (number? type) type @(ns-resolve type 'TYPE)) aseq (count aseq))))

(defn alength
  "Returns the size of an array. For multidimensional arrays, returns the last dimension."
  [a] (count a))

(def subvec -slice)
(def subs -slice)

(def aset-boolean aset)
(def aset-byte aset)
(def aset-double aset)
(def aset-float aset)
(def aset-int aset)
(def aset-short aset)

; Namespaces

(defn ns-resolve
  [ns sym] (if (namespace sym)
             (find (car (find-ns (symbol (namespace sym)))) (symbol (name sym)))
             (find (car ns) sym)))

(defn resolve
  [sym] (ns-resolve *ns* sym))

(def-macro (var symbol) `(or (and (namespace '~symbol)
                                  (ns-resolve (find-ns (symbol (namespace '~symbol))) (symbol (name '~symbol))))
                             (resolve '~symbol)
                             (throw (new RuntimeException (str "Use of undeclared Var " '~symbol)))))

(defn ns-map
  "Returns the var-map of namespace ns"
  [ns] (car ns))

(defn the-ns
  "Returns the namespace that the symbol x refers, or x itself, if x is a namespae"
  [x] (if (isa? clojure.lang.Namespace x)
        x
        (or (find-ns x) (throw (new RuntimeException "No namespace found")))))

(defn alias
  "Adds an alias in the current namespace to another namespace"
  [alias-sym ns-sym] (intern *ns* (Alias alias-sym) (the-ns ns-sym)))
