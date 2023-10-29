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

## Track CPU load in a plot

## Load CSV file

## Load and visualize GraphML file

## Draw a rotating supershape
