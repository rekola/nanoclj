;    Initialization file for NanoClojure 1.0

(load "bootstrap.clj")
(load "clojure.core.clj")

(macro (unless form)
     `(if (not ,(cadr form)) (do ,@(cddr form))))

(macro (when form)
       `(if ,(cadr form) (do ,@(cddr form))))

(macro (delay form) `(clojure.lang.Delay ',(cdr form)))

;;;;; I/O

(defn call-with-input-file [s p]
  (let [inport (clojure.lang.io/reader s)]
    (if (= inport false)
      false
      (let [res (p inport)]
        (close inport)
        res))))

(defn call-with-output-file [s p]
  (let [outport (clojure.lang.io/writer s)
        old-outport *out*]
    (if outport
      (do (set! *out* outport)
          (let [res (p)]
            (close outport)
            (set! *out* old-outport)
            res
            ))
      false)))

(defn with-input-from-file [s p]
  (let [inport (clojure.lang.io/reader s)]
    (if (= inport false)
      false
      (let [prev-inport *in*]
        (set! *in* inport)
        (let ((res (p)))
          (close inport)
          (set! *in* prev-inport)
          res)))))

(defn with-output-to-file [s p]
  (let [outport (clojure.lang.io/writer s)]
    (if (= outport false)
      false
      (let [prev-outport *out*]
        (set! *out* outport)
        (let ((res (p)))
          (close outport)
          (set! *out* prev-outport)
          res)))))

(defn with-input-output-from-to-files [si so p]
  (let [inport (clojure.lang.io/reader si)
        outport (clojure.lang.io/writer so)]
    (if (not (and inport outport))
      (do
        (close inport)
        (close outport)
        false)
      (let [prev-inport *in*
            prev-outport *out*]
        (set! *in* inport)
        (set! *out* outport)
        (let [res (p)]
          (close inport)
          (close outport)
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

(defn true? [x] (equals? x true))
(defn false? [x] (equals? x false))

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

; Lists

(defn filter [pred coll]
  (cond (empty? coll) '()
        (pred (first coll)) (cons (first coll) (filter pred (rest coll)))
        :else (filter pred (rest coll))))

(defn remove [pred coll]
  (cond (empty? coll) '()
        (pred (first coll)) (remove pred (rest coll))
        :else (cons (first coll) (remove pred (rest coll)))))

(defn ns-interns [ns] (doall (map first (reduce concat '() (car ns)))))

; Atoms

; (defn swap! [atom f x] (set! atom (f atom x)))
; add reset!
; add compare-and-set!

(macro (package form) `(apply (fn [] ,@(cdr form) *ns*) '()))

(load "clojure.lang.io.clj")
(load "clojure.string.clj")
(load "clojure.repl.clj")
(load "Integer.clj")
(load "Long.clj")
(load "sparkline.clj")
(load "Plot.clj")
