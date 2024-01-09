# Examples

## Load an image from the net and show it

```clojure
(require 'nanoclj.image)
(nanoclj.image/load "https://picsum.photos/500")
```

## Draw two plots

```clojure
(require '[nanoclj.plot :as p])
(def X (p/linspace 0 (* Math/PI 2))
(def Y1 (mapv Math/sin X))
(def Y2 (mapv Math/cos X))
(p/plot X Y1 X Y2)
```

## Draw a scatter plot

```clojure
(require '[nanoclj.plot :as p])
(def X (p/linspace 0 (* Math/PI 3)))
(def Y (mapv #( + (Math/cos %1) (rand)) X))
(p/scatter X Y)
```

## Plot a CSV file

First load and unzip ECB data: https://www.ecb.europa.eu/stats/eurofxref/eurofxref-hist.zip

```clojure
(require '[nanoclj.plot :as p])
(def data (rest (clojure.data.csv/read-csv (clojure.java.io/reader "eurofxref-hist.csv"))))
(def dates (map #( clojure.instant/read-instant-date (%1 0) ) data))
(def USD (map #( java.lang.Double/parseDouble (%1 1) ) data))
(p/plot dates USD)
```

## Load and visualize a GraphML file

```clojure
(require '[nanoclj.plot :as p] '[nanoclj.graph :as g])
(def G (g/load "karate.graphml"))
(p/graph-plot G)
```

## Create a PDF

```clojure
(with-out-pdf "test.pdf" 612 828
   (println "Hello world!"))
```

## Track CPU load in a plot

## Draw a rotating supershape
