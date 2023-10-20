;    Initialization file for NanoClojure 1.0

(load-file "bootstrap.clj")
(load-file "clojure.core.clj")

;;;;; I/O

(defn call-with-input-file [s p]
  (let [inport (clojure.java.io/reader s)]
    (if (= inport false)
      false
      (let [res (p inport)]
        (-close inport)
        res))))

(defn call-with-output-file [s p]
  (let [outport (clojure.java.io/writer s)
        old-outport *out*]
    (if outport
      (do (set! *out* outport)
          (let [res (p)]
            (-close outport)
            (set! *out* old-outport)
            res
            ))
      false)))

(defn with-input-from-file [s p]
  (let [inport (clojure.java.io/reader s)]
    (if (= inport false)
      false
      (let [prev-inport *in*]
        (set! *in* inport)
        (let [res (p)]
          (-close inport)
          (set! *in* prev-inport)
          res)))))

(defn with-output-to-file [s p]
  (let [outport (clojure.java.io/writer s)]
    (if (= outport false)
      false
      (let [prev-outport *out*]
        (set! *out* outport)
        (let [res (p)]
          (-close outport)
          (set! *out* prev-outport)
          res)))))

(defn with-input-output-from-to-files [si so p]
  (let [inport (clojure.java.io/reader si)
        outport (clojure.java.io/writer so)]
    (if (not (and inport outport))
      (do
        (-close inport)
        (-close outport)
        false)
      (let [prev-inport *in*
            prev-outport *out*]
        (set! *in* inport)
        (set! *out* outport)
        (let [res (p)]
          (-close inport)
          (-close outport)
          (set! *in* prev-inport)
          (set! *out* prev-outport)
          res)))))

; Random number generator (maximum cycle)
(def *seed* 1)
(defn rand- []
  (let [a 16807
        m 2147483647
        q (quot m a)
        r (mod m a)]
    (set! *seed*
          (-   (* a (- *seed*
                       (* (quot *seed* q) q)))
               (* (quot *seed* q) r)))
    (if (< *seed* 0) (set! *seed* (+ *seed* m)))
    *seed*))

(def rand (fn
            ([] (rand 1))
            ([n] (* n (/ (rand-) 2147483647.0)))))

(defn rand-int [n] (mod (rand-) n))

(def nth
  "Returns the nth element of coll"
  (fn
    ([coll index]
     (cond (vector? coll) (coll index)
     	   (string? coll) (recur (seq coll) index)
     	   (or (< index 0) (empty? coll)) (throw "ERROR - Index out of bounds")
           (if (zero? index) (first coll) (recur (next coll) (dec index)))))
    ([coll index not-found]
     (cond (vector? coll) (coll index not-found)
     	   (string? coll) (recur (seq coll) index not-found)
     	   (or (< index 0) (empty? coll)) not-found
     	   :else (if (zero? index) (first coll) (recur (next coll) (dec index) not-found))))))

(defn rand-nth [coll] (nth coll (rand-int (count coll))))

(defn third [coll] (first (next (next coll))))

(defn ns-interns [ns] (doall (map first (reduce concat '() (car ns)))))

(load-file "clojure.java.io.clj")
(load-file "clojure.java.shell.clj")
(load-file "clojure.string.clj")
(load-file "clojure.repl.clj")
(load-file "java.lang.Object.clj")
(load-file "java.lang.Integer.clj")
(load-file "java.lang.Long.clj")
(load-file "java.lang.String.clj")
(load-file "java.util.Date.clj")
(load-file "sparkline.clj")
(load-file "Plot.clj")
(load-file "nanoclj.core.Codepoint.clj")
(load-file "nanoclj.core.Tensor.clj")

