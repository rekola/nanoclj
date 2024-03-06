; Initialization file for nanoclj

(load-file "bootstrap.clj")

(import java.lang.Float)

; Most of the classes have already been defined, so :reload flag is needed to trigger the load
(require 'java.lang.Object :reload :import)
(require 'java.lang.Class :reload :import)
(require 'java.lang.Integer :reload :import)
(require 'java.lang.Number :reload :import)
(require 'java.lang.Double :reload :import)
(require 'java.lang.Boolean :reload :import)
(require 'java.lang.Byte :reload :import)
(require 'java.lang.Short :reload :import)
(require 'java.lang.Long :reload :import)
(require 'java.lang.String :reload :import)
(require 'java.util.Date :reload :import)
(require 'java.util.UUID :reload :import)
(require 'java.io.Reader :reload :import)
(require 'java.io.Writer :reload :import)
(require 'clojure.lang.PersistentVector :reload :import)
(require 'clojure.lang.PersistentQueue :reload :import)
(require 'nanoclj.lang.Codepoint :reload :import)
(require 'nanoclj.lang.Tensor :reload :import)
(require 'nanoclj.lang.Image :reload :import)

(load-file "clojure.core.clj")
