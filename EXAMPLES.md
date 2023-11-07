# Examples

## Load an image from the net and show it

```clojure
(nanoclj.image/load "https://picsum.photos/500")
```

## Draw two plots

```clojure
(def X (linspace 0 (* Math/PI 2))
(def Y1 (mapv Math/sin X))
(def Y2 (mapv Math/cos X))
(plot X Y1 X Y2)
```

## Draw a scatter plot

```clojure
(def X (linspace 0 (* Math/PI 3)))
(def Y (mapv #( + (Math/cos %1) (rand)) X))
(scatter X Y)
```

## Plot a CSV file

First load and unzip ECB data: https://www.ecb.europa.eu/stats/eurofxref/eurofxref-hist.zip

```clojure
(def data (rest (clojure.data.csv/read-csv (clojure.java.io/reader "eurofxref-hist.csv"))))
(def dates (map #( clojure.instant/read-instant-date (%1 0) ) data))
(def USD (map #( java.lang.Double/parseDouble (%1 1) ) data))
(plot dates USD)
```

## Load and visualize a GraphML file

```clojure
(def G (nanoclj.graph/load "karate.graphml"))
(nanoclj.graph/update-layout G)
(graph-plot G)
```

## Create a PDF

## Track CPU load in a plot

## Draw a rotating supershape
