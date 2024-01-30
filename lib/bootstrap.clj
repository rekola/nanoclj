(require 'clojure.lang.Symbol :import)
(require 'clojure.lang.PersistentVector :import)
(require 'clojure.lang.Cons :import)
(require 'nanoclj.core.Macro :import)
(require 'nanoclj.core.Procedure :import)
(require 'nanoclj.core.Closure :import)
(require 'nanoclj.core.ForeignFunction :import)
(require 'java.lang.Number :import)
(require 'java.lang.String :import)

(def force deref)
(def var-get second)

(def car first)
(def cdr rest)
(def cadr second)
(def cddr (fn [x] (rest (rest x))))
(def caadr (fn [x] (first (second x))))
(def cdadr (fn [x] (rest (second x))))
(def caddr (fn [x] (car (cdr (cdr x)))))
(def cadddr (fn [x] (car (cdr (cdr (cdr x))))))

; (macro (my-intern- form) (cons 'def (cons (caddr form) (cons (cadddr form) '()))))
; (def my-intern (fn [ns name val] (let [s name] (my-intern- ns s val))))

(def symbol clojure.lang.Symbol)
(def vector clojure.lang.PersistentVector)
(def cons clojure.lang.Cons)

(def nil?
  "Returns true if the argument is nil"
  (fn [x] (identical? x nil)))

(def some?
  "Returns true if the argument is not nil"
  (fn [x] (not (identical? x nil))))

(def list? (fn [x] (instance? clojure.lang.Cons x)))
(def zero? (fn [n] (equiv? n 0)))

(def ^:private is-any-of?
  (fn
    ([x] false)
    ([x a] (equals? x a))
    ([x a b] (or (equals? x a) (equals? x b)))
    ([x a b c] (or (equals? x a) (equals? x b) (equals? x c)))
    ([x a b c & args] (or (equals? x a) (equals? x b) (equals? x c) (apply* is-any-of? (cons x args))))))

(def int?
  "Returns true if the argument is fixed-precision integer"
  (fn [x] (is-any-of? (type x) java.lang.Byte java.lang.Short java.lang.Integer java.lang.Long)))

(def integer?
  "Returns true if the argument is integer"
  (fn [x] (is-any-of? (type x) java.lang.Byte java.lang.Short java.lang.Integer java.lang.Long clojure.lang.BigInt)))

(def number?
  "Returns true if the argument is a number"
  (fn [x] (instance? java.lang.Number x)))

(def string?
  "Returns true if argument is a string"
  (fn [x] (instance? java.lang.String x)))

(def symbol?
  "Returns true if argument is a symbol"
  (fn [x] (instance? clojure.lang.Symbol x)))

(def var?
  "Returns true if argument is a var"
  (fn [x] (instance? clojure.lang.Var x)))

(def char?
  "Returns true if argument is a character (actually a utf8 codepoint)"
  (fn [x] (instance? nanoclj.core.Codepoint x)))

(def file?
  "Returns true if argument is a File"
  (fn [x] (instance? java.io.File x)))

(def procedure?
  "Returns true if the argument is a function"
  (fn [x] (is-any-of? (type x) nanoclj.core.Procedure nanoclj.core.Closure nanoclj.core.ForeignFunction)))

(def macro?
  "Returns true if the argument is a macro"
  (fn [x] (instance? nanoclj.core.Macro x)))

(def sorted?
  "Returns true if coll is a sorted collection"
  (fn [x] (or (instance? clojure.lang.PersistentTreeSet) (instance? cloure.lang.PersistentTreeMap))))

(def class?
  "Returns true if x is a Class"
  (fn [x] (instance? java.lang.Class x)))

(def ifn?
  "Returns true if coll is invokeable"
  (fn [x] (instance? clojure.lang.AFn x)))

(def list
  "Creates a list from the given args"
  (fn [& args] (or args '())))

(def spread
  (fn [args] (cond
               (nil? args) nil
               (nil? (next args)) (seq (first args))
               :else (cons (first args) (spread (next args))))))

(def list*
  "Creates a list with the initial arguments prepended to the last argument"
  (fn
    ([args] (seq args))
    ([a args] (cons a args))
    ([a b args] (cons a (cons b args)))
    ([a b c args] (cons a (cons b (cons c args))))
    ([a b c d & more] (cons a (cons b (cons c (cons d (spread more))))))))

(def apply
  "Calls the function f with the given args"
  (fn
    ([f args] (apply* f (seq args)))
    ([f a args] (apply* f (list* a args)))
    ([f a b args] (apply* f (list* a b args)))
    ([f a b c args] (apply* f (list* a b c args)))
    ([f a b c d & args] (apply* f (cons a (cons b (cons c (cons d (spread args)))))))))

;; The following quasiquote macro is due to Eric S. Tiedemann.
;;   Copyright 1988 by Eric S. Tiedemann; all rights reserved.
;;
;; Subsequently modified to handle vectors: D. Souflis

(macro quasiquote
 (fn [l]
   (let [mcons (fn [f l r]
                 (if (and (list? r)
                          (equals? (car r) 'quote)
                          (equals? (car (cdr r)) (cdr f))
                          (list? l)
                          (equals? (car l) 'quote)
                          (equals? (car (cdr l)) (car f)))
                   (if (or (procedure? f) (number? f) (string? f))
                     f
                     (list 'quote f))
                   (if (equals? l vector)
                     (apply l (eval r))
                     (list 'cons l r)
                     )))
         mappend (fn [f l r]
                   (if (or (empty? (cdr f))
                           (and (list? r)
                                (equals? (car r) 'quote)
                                (equals? (car (cdr r)) '())))
                     l
                     (list 'concat l r)))
         foo (fn [level form]
               (cond (not (list? form))
                     (if (or (procedure? form) (number? form) (string? form))
                       form
                       (list 'quote form))
               
                     (equals? 'quasiquote (car form))
                     (mcons form ''quasiquote (foo (+ level 1) (cdr form)))
                     :else (if (zero? level)
                             (cond (equals? (car form) 'unquote) (car (cdr form))
                                   (equals? (car form) 'unquote-splicing)
                                   (throw (new RuntimeException (str "Unquote-splicing wasn't in a list:" form)))
                                   (and (list? (car form))
                                        (equals? (car (car form)) 'unquote-splicing))
                                   (mappend form (car (cdr (car form)))
                                            (foo level (cdr form)))
                                   :else (mcons form (foo level (car form))
                                                (foo level (cdr form))))
                             (cond (equals? (car form) 'unquote)
                                   (mcons form ''unquote (foo (- level 1)
                                                              (cdr form)))
                                   (equals? (car form) 'unquote-splicing)
                                   (mcons form ''unquote-splicing
                                          (foo (- level 1) (cdr form)))
                                   :else (mcons form (foo level (car form))
                                                (foo level (cdr form)))))))
         ]
     (foo 0 (car (cdr l))))))

; DEFINE-MACRO Contributed by Andy Gaynor
(macro def-macro (fn [dform]
                   (if (symbol? (cadr dform))
                     `(macro ~@(cdr dform))
                     (let [form (gensym)]
                       `(macro (~(caadr dform) ~form)
                               (apply (fn ~(cdadr dform) ~@(cddr dform)) (cdr ~form)))))))

;;;; Utility to ease macro creation
(def macroexpand (fn [form] ((eval (source (eval (car form)))) form)))

(def macro-expand-all (fn [form]
                        (if (macro? form)
                          (macro-expand-all (macroexpand form))
                          form)))

(def *compile-hook* macro-expand-all)

; (macro (defn form) `(intern *ns* '~(cadr form) (fn ~(caddr form) ~(cadddr form))))
; (macro my-def (fn [name expression] (cons 'intern (cons '*ns* (cons (symbol name) (cons 1 '()))))))

; (macro (defn-2 form) `(intern *ns* '~(cadr form) (fn ~(cddr form))))
; (def-macro (defn name params body) `(intern *ns* '~name (fn ~params ~body)))
; (def-macro (defn name params & body) `(intern *ns* '~name (fn ~params ~@body)))
(def-macro (defn name & args) (if (string? (car args))
                                `(def ~name ~(car args) (fn ~(cadr args) ~@(cddr args)))
                                `(def ~name (fn ~(car args) ~@(cdr args)))))
(def-macro (defn- name & args) (if (string? (car args))
                                 `(def ^:private ~name ~(car args) (fn ~(cadr args) ~@(cddr args)))
                                 `(def ^:private ~name (fn ~(car args) ~@(cdr args)))))
; (def-macro (my-cond & form) (if (empty? form) '() `(if ~(car form) ~(cadr form) nil)))
; (macro (my-cond form) (if (empty? (cdr form)) '() (cons 'if (cons (cadr form) (cons (caddr form) (my-cond (cdddr form)))))))


(def-macro (set! symbol value) `(-set '~symbol ~value))
(def-macro (. instance symbol & args) `(-dot ~instance '~symbol ~@args))

(defn foldr [f x lst]
  (if (empty? lst)
    x
    (foldr f (f x (car lst)) (cdr lst))))

(def-macro (case e & clauses)
  (if (empty? clauses) `(throw (new RuntimeException (str "No matching clause: " ~e)))
      (if (empty? (rest clauses))
        (first clauses)
        `(if (equals? ~e ~(car clauses))
           ~(cadr clauses)
           (case ~e ~@(cddr clauses))))))
  
; Add multiarity versions of basic operators

(def reduce
  "Reduces a collection by applying f"
  (fn
    ([f coll] (if (empty? coll)
                (f)
                (if (empty? (rest coll))
                  (first coll)
                  (reduce f (f (first coll) (second coll)) (rest (rest coll))))))
    ([f val coll] (if (empty? coll)
                    val
                    (reduce f (f val (first coll)) (rest coll))))))

(defn max
  "Returns the maximum of the arguments"
  ([x] x)
  ([x y] (if (gt x y) x y))
  ([x y & args] (reduce max (max x y) args)))

(defn min
  "Returns the minimum of the arguments"
  ([x] x)
  ([x y] (if (lt x y) x y))
  ([x y & args] (reduce min (min x y) args)))

(defn +
  "Returns the sum of the arguments. Doesn't auto-promote longs."
  ([x] x)
  ([x y] (add x y))
  ([x y & more] (reduce add (add x y) more)))

(defn +'
  "Returns the sum of the arguments. Supports arbitrary precision."
  ([x] x)
  ([x y] (add' x y))
  ([x y & more] (reduce add' (add' x y) more)))

(defn *
  "Returns the product of the arguments. Doesn't auto-promote longs."
  ([x] x)
  ([x y] (multiply x y))
  ([x y & more] (reduce multiply (multiply x y) more)))

(defn *'
  "Returns the product of the arguments. Supports arbitrary precision."
  ([x] x)
  ([x y] (multiply' x y))
  ([x y & more] (reduce multiply' (multiply' x y) more)))

(defn -
  "Subtracts the arguments. Doesn't auto-promote longs."
  ([x] (minus 0 x))
  ([x y] (minus x y))
  ([x y & more] (reduce minus (minus x y) more)))

(defn -'
  "Subtracts the arguments. Supports arbitrary precision."
  ([x] (minus' 0 x))
  ([x y] (minus' x y))
  ([x y & more] (reduce minus' (minus' x y) more)))

(defn /
  "Divides the arguments"
  ([x] (divide 1 x))
  ([x y] (divide x y))
  ([x y & more] (reduce divide (divide x y) more)))

(defn bit-or
  "Returns a bitwise or"
  ([x y] (-bit-or x y))
  ([x y & more] (reduce -bit-or (-bit-or x y) more)))

(defn bit-and
  "Returns a bitwise and"
  ([x y] (-bit-and x y))
  ([x y & more] (reduce -bit-and (-bit-and x y) more)))

(defn bit-xor
  "Returns a bitwise xor"
  ([x y] (-bit-xor x y))
  ([x y & more] (reduce -bit-xor (-bit-xor x y) args)))

(defn bit-not 
  "Returns a bitwise not"
  [x] (- 0 (inc x)))

(defn bit-and-not
  "Returns bitwise and with complement"
  ([x y] (-bit-and x (bit-not y)))
  ([x y & more] (reduce #( -bit-and %1 (bit-not %2) ) (-bit-and x (bit-not y)) more)))

(defn bit-test
  "Test a bit of x at index n"
  [x n] (if (equiv? (bit-and- x (bit-shift-left 1 n)) 0) false true))

(defn bit-set
  "Sets a bit of x at index n"
  [x n] (bit-or- x (bit-shift-left 1 n)))

(defn bit-clear
  "Clears a bit of x at index n"
  [x n] (bit-and- x (bit-not (bit-shift-left 1 n))))

(defn bit-flip
  "Flips a bit of x at index n"
  [x n] (bit-xor- x (bit-shift-left 1 n)))

(defn conj
  ([coll] coll)
  ([coll x] (-conj coll x))
  ([coll x & xs] (reduce -conj (-conj coll x) xs)))

(defn =
  "Returns true if the arguments are equal"
  ([x] true)
  ([x y] (equals? x y))
  ([x y & more] (if (equals? x y)
                  (if (next more)
                    (recur y (first more) (next more))
                    (equals? y (first more)))
                  false)))
  
(defn ==
  "Returns true if the arguments are numerically equal"
  ([x] true)
  ([x y] (equiv? x y))
  ([x y & more] (if (equiv? x y)
                  (if (next more)
                    (recur y (first more) (next more))
                    (equiv? y (first more)))
                  false)))

(defn <
  "Returns true if the arguments are in ascending order"
  ([x] true)
  ([x y] (lt x y))
  ([x y & more] (if (lt x y)
                  (if (next more)
                    (recur y (first more) (next more))
                    (lt y (first more)))
                  false)))

(defn >
  "Returns true if the arguments are in descending order"
  ([x] true)
  ([x y] (gt x y))
  ([x y & more] (if (gt x y)
                  (if (next more)
                    (recur y (first more) (next more))
                    (gt y (first more)))
                  false)))

(defn <=
  ([x] true)
  ([x y] (le x y))
  ([x y & more] (if (le x y)
                  (if (next more)
                    (recur y (first more) (next more))
                    (le y (first more)))
                  false)))

(defn >=
  ([x] true)
  ([x y] (ge x y))
  ([x y & more] (if (ge x y)
                  (if (next more)
                    (recur y (first more) (next more))
                    (ge y (first more)))
                  false)))

(defn not=
  "Returns true if the arguments are not equal"
  ([x] false)
  ([x y] (not (equals? x y)))
  ([x y & more] (not (apply equals? x y more))))

(defn closure?
  "Returns true if argument is a closure. Note, a macro object is also a closure."
  [x] (instance? nanoclj.core.Closure x))

(macro when (fn [form]
              `(if ~(cadr form) (do ~@(cddr form)))))

(macro when-not (fn [form]
                  `(if (not ~(cadr form)) (do ~@(cddr form)))))

(macro if-not (fn [form]
                `(if ~(cadr form) ~(cadddr form) ~(caddr form))))

(macro delay (fn [form] `(clojure.lang.Delay '~(cdr form))))

(def-macro (ns name) `(in-ns '~name))

(def-macro (import name) `(require '~name :import))
