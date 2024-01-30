; Initialization file for nanoclj

(load-file "bootstrap.clj")
(load-file "clojure.core.clj")
(load-file "clojure.repl.clj")

; Most of the classes have already been defined, so :reload flag is needed to trigger the load
(require 'java.lang.Object :reload)
(require 'java.lang.Class :reload)
(require 'java.lang.Integer :reload)
(require 'java.lang.Float :reload)
(require 'java.lang.Double :reload)
(require 'java.lang.Boolean :reload)
(require 'java.lang.Byte :reload)
(require 'java.lang.Short :reload)
(require 'java.lang.Long :reload)
(require 'java.lang.String :reload)
(require 'java.util.Date :reload)
(require 'java.util.UUID :reload)
(require 'clojure.lang.PersistentVector :reload)
(require 'nanoclj.core.Codepoint :reload)
(require 'nanoclj.core.Tensor :reload)
(require 'nanoclj.core.Image :reload)
