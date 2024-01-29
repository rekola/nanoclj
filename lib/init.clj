; Initialization file for nanoclj

(load-file "bootstrap.clj")
(load-file "clojure.core.clj")
(load-file "clojure.repl.clj")

; Since these types are already defined natively, the cannot be loaded with require
(load-file "java.lang.Object.clj")
(load-file "java.lang.Class.clj")
(load-file "java.lang.Integer.clj")
(load-file "java.lang.Double.clj")
(load-file "java.lang.Boolean.clj")
(load-file "java.lang.Byte.clj")
(load-file "java.lang.Short.clj")
(load-file "java.lang.Long.clj")
(load-file "java.lang.String.clj")
(load-file "java.util.Date.clj")
(load-file "java.util.UUID.clj")
(load-file "clojure.lang.PersistentVector.clj")
(load-file "nanoclj.core.Codepoint.clj")
(load-file "nanoclj.core.Tensor.clj")
(load-file "nanoclj.core.Image.clj")

(require 'java.lang.Float)
