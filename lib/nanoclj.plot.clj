(in-ns 'nanoclj.plot)

(def ^:private plot-colors '([0 0.4470 0.7410]
                             [0.8500 0.3250 0.0980]
                             [0.9290 0.6940 0.1250]
                             [0.4940 0.1840 0.5560]
                             [0.4660 0.6740 0.1880]
                             [0.3010 0.7450 0.9330]
                             [0.6350 0.0780 0.1840]
                             ))

(def ^:private theme-colors {
                             :dark {
                                    :text [ 0.5 0.54 0.7 ]
                                    :box [ 0.3 0.32 0.4 ]
                                    :grid [ 0.2 0.21 0.3 ]
                                    :bg [ 0.12 0.14 0.2 ]
                                    }
                             :bright {
                                      :text [ 0.5 0.5 0.5 ]
                                      :box [ 0.6 0.6 0.6 ]
                                      :grid [ 0.7 0.7 0.7 ]
                                      :bg [ 1 1 1 ] } })

(defn linspace
  "Creates an evenly spaced vector from a to b with n points"
  ([a b] (linspace a b 100))
  ([a b n] (let [n (dec n)
                 step (/ (- b a) (double n))
                 ]
             (loop [a (+ a step) n (dec n) acc [a]]
               (if (> n 0)
                 (recur (+ a step) (dec n) (conj acc a))
                 (conj acc b))))))

(defn graph-plot
  "Plots a map"
  ([& graphs]
   (let [width (*window-size* 0)
         height (int (/ width 3))
         margin-top 10
         margin-bottom 10
         margin-left 10
         margin-right 10
         content-width (- width margin-left margin-right)
         content-height (- height margin-top margin-bottom)
         min-x -100
         min-y -100
         max-x 100
         max-y 100
         range-x (- max-x min-x)
         range-y (- max-y min-y)
         fit-x (fn [x] (+ (* (/ (- x min-x) range-x) content-width) margin-left))
         fit-y (fn [y] (+ (* (/ (- max-y y) range-y) content-height) margin-top))
         cx (clojure.java.io/writer width height)
         pi-times-2 (* 2 Math/PI)
         draw-edge (fn [g e] (let [[x0 y0] ((g (e :source)) :position)
                                   [x1 y1] ((g (e :target)) :position)
                                   ]
                               (set-line-width 5)
                               (new-path)
                               (move-to (fit-x x0) (fit-y y0))
                               (line-to (fit-x x1) (fit-y y1))
                               (stroke)
                               ))
         draw-node (fn [n] (let [[x0 y0] (n :position)
                                 data (val (n :data))
                                 label (get data "name")
                                 x (fit-x x0)
                                 y (fit-y y0)]
                             (set-color [ 1.0 0.5 0.5 ])
                             (new-path)
                             (arc x y 6 0 pi-times-2)
                             (fill)
                             (set-color [ 0.0 0.0 0.0 0.5 ])
                             (new-path)
                             (arc x y 6 0 pi-times-2)
                             (set-line-width 1)
                             (stroke)
                             (when label
                               (move-to x y)
                               (print label))
                             ))
         draw-graph (fn [g]
                      (.updateLayout g)
                      (set-color [ 0 0 0 0.5 ])
                      (run! #( draw-edge g %1 ) (g :edges))
                      (run! draw-node g))
         draw (fn []
                (set-font-size 10)
                (run! draw-graph graphs))
         ]
     (with-out cx
       (draw)
         ))))

(defn- parse-args
  [args] (loop [a args r { :data [] }] (if a (if (keyword? (first a))
                                               (recur (nnext a) (assoc r (first a) (second a)))
                                               (recur (next a) (assoc r :data (conj (r :data) (first a)))))
                                           r)))

(defn plot
  "Plots a series. Matlab style."
  ([& args]
   (let [args (parse-args (seq args))
         width (*window-size* 0)
         height (int (/ width 3))
         theme (theme-colors *theme*)
         text-color (theme :text)
         box-color (theme :box)
         grid-color (theme :grid)
         title (get args :title)
         plots (map #( conj (vec %1) %2 ) (partition 2 (args :data)) plot-colors)
         margin-top (if title 30 15)
         margin-bottom 25
         margin-left 50
         margin-right 15
         content-width (- width margin-left margin-right)
         content-height (- height margin-top margin-bottom)
         min-x (apply min (map #( apply min (first %1) ) plots))
         min-y (apply min (map #( apply min (second %1) ) plots))
         max-x (apply max (map #( apply max (first %1) ) plots))
         max-y (apply max (map #( apply max (second %1) ) plots))
         range-x (- (double max-x) (double min-x))
         range-y (- max-y min-y)
         fit-x (fn [x] (+ (* (/ (- (double x) (double min-x)) range-x) content-width) margin-left))
         fit-y (fn [y] (+ (* (/ (- max-y y) range-y) content-height) margin-top))
         x-tick-step (Math/pow 10 (dec (Math/round (Math/log10 range-x))))
         y-tick-step (Math/pow 10 (dec (Math/round (Math/log10 (* range-y 2)))))
         x-grid-step (/ x-tick-step 2.0)
         y-grid-step (/ y-tick-step 2.0)
         format-x (case (class min-x)
                    java.util.Date (fn [x] (str (java.util.Date x)))
                    (fn [x] (format "%.2f" x)))
         format-y (fn [x] (format "%.2f" x))
         draw-x-grid (fn [x] (when (<= x max-x)
                               (let [fx (fit-x x)]
                                 (move-to fx margin-top)
                                 (line-to fx (- height margin-bottom))
                                 (stroke)
                                 (recur (+ x x-grid-step)))))
         draw-y-grid (fn [y] (when (<= y max-y)
                               (let [fy (fit-y y)]
                                 (move-to margin-left fy)
                                 (line-to (- width margin-right) fy)
                                 (stroke)
                                 (recur (+ y y-grid-step)))))
         draw-x-ticks (fn [x] (when (<= x max-x)
                                (let [fx (fit-x x)
                                      label (format-x x)
                                      e (get-text-extents label)]
                                  (set-color box-color)
                                  (move-to fx (- height margin-bottom))
                                  (line-to fx (- height margin-bottom -3))
                                  (stroke)
                                  (set-color text-color)
                                  (move-to (- fx (/ (e 0) 2)) (- height (/ margin-bottom 2)))
                                  (print label)
                                  (recur (+ x x-tick-step)))))
         draw-y-ticks (fn [y] (when (<= y max-y)
                                (let [fy (fit-y y)
                                      label (format-y y)
                                      e (get-text-extents label)]
                                  (set-color box-color)
                                  (move-to margin-left fy)
                                  (line-to (- margin-left 3) fy)
                                  (stroke)
                                  (set-color text-color)
                                  (move-to (- margin-left (e 0) 5) (+ fy (/ (e 1) 2)))
                                  (print label)
                                  (recur (+ y y-tick-step)))))
         draw-line (fn [x y] (if (or (empty? x) (empty? y))
                               nil
                               (do
                                 (line-to (fit-x (first x)) (fit-y (first y)))
                                 (recur (rest x) (rest y)))))
         cx (clojure.java.io/writer width height (theme :bg))
         draw (fn []
                (set-font-size 10)
                                        ; Draw grid
                (set-color grid-color)
                (set-line-width :hair)
                (draw-x-grid (* (int (/ (double min-x) x-grid-step)) x-grid-step))
                (draw-y-grid (* (int (/ min-y y-grid-step)) y-grid-step))
                                        ; Draw box
                (set-color box-color)
                (set-line-width 1)
                (move-to margin-left margin-top)
                (line-to (- width margin-right) margin-top)
                (line-to (- width margin-right) (- height margin-bottom))
                (line-to margin-left (- height margin-bottom))
                (close-path)
                (stroke)
                                        ; Draw ticks
                (draw-x-ticks (* (int (/ (double min-x) x-tick-step)) x-tick-step))
                (draw-y-ticks (* (int (/ min-y y-tick-step)) y-tick-step))

                                        ; Draw shading
                (run! (fn [p]
                        (let [[x y color] p
                              x2 (reverse x)
                              y2 (map #( * 0.8 %1) (reverse y))
                              ]
                          (set-color (conj color 0.25))
                          (move-to (fit-x (first x)) (fit-y (first y)))
                          (draw-line (rest x) (rest y))
                          (draw-line x2 y2)
                          (fill))) plots)
                                        ; Draw the plots
                (set-line-width 2)
                (run! (fn [p]
                        (let [ [x y color] p ]
                          (set-color color)
                          (move-to (fit-x (first x)) (fit-y (first y)))
                          (draw-line (rest x) (rest y))
                          (stroke))) plots)

                (when title
                  (let [title-size (get-text-extents title)]
                    (set-font-size 15)
                    (set-color text-color)
                    (move-to (+ margin-left (/ content-width 2) (/ (title-size 0) -2)) (+ 15 (/ (title-size 1) 2)))
                    (print title)))
                )
         ]
     (with-out cx
       (draw)
         
       (add-watch (var *window-size*) 0
                  (fn [key ref old-ws ws]
                    (with-out cx
                      (resize)
                      (flush)
                      )))
       
       (add-watch (var *mouse-pos*) 0
                  (fn [key ref old-pos pos]
                    nil
                    ))
       
       ))))

(defn scatter
  "Draws a scatter plot. Matlab style."
  ([& args]
   (let [width (*window-size* 0)
         height (int (/ width 3))
         box-color ((theme-colors *theme*) :box)
         plots (map #( conj (vec %1) %2 ) (partition 2 args) plot-colors)
         margin-top 15
         margin-bottom 25
         margin-left 50
         margin-right 15
         content-width (- width margin-left margin-right)
         content-height (- height margin-top margin-bottom)
         min-x (apply min (map #( apply min (first %1) ) plots))
         min-y (apply min (map #( apply min (second %1) ) plots))
         max-x (apply max (map #( apply max (first %1) ) plots))
         max-y (apply max (map #( apply max (second %1) ) plots))
         range-x (- max-x min-x)
         range-y (- max-y min-y)
         fit-x (fn [x] (+ (* (/ (- x min-x) range-x) content-width) margin-left))
         fit-y (fn [y] (+ (* (/ (- max-y y) range-y) content-height) margin-top))
         x-tick-step 0.25
         y-tick-step 0.25
         pi-times-2 (* 2 Math/PI)
         draw-x-ticks (fn [x] (if (<= x max-x)
                                (let [fx (fit-x x)
                                      label (format "%.2f" x)
                                      e (get-text-extents label)]
                                  (move-to fx margin-top)
                                  (line-to fx (+ margin-top 3))
                                  (stroke)
                                  (move-to fx (- height margin-bottom))
                                  (line-to fx (- height margin-bottom 3))
                                  (stroke)
                                  (move-to (- fx (/ (e 0) 2)) (- height (/ margin-bottom 2)))
                                  (print label)
                                  (recur (+ x x-tick-step)))
                                nil))
         draw-y-ticks (fn [y] (if (<= y max-y)
                                (let [fy (fit-y y)
                                      label (format "%.2f" y)
                                      e (get-text-extents label)]
                                  (move-to margin-left fy)
                                  (line-to (+ margin-left 3) fy)
                                  (stroke)
                                  (move-to (- width margin-right) fy)
                                  (line-to (- width margin-right 3) fy)
                                  (stroke)
                                  (move-to (- margin-left (e 0) 5) (+ fy (/ (e 1) 2)))
                                  (print label)
                                  (recur (+ y y-tick-step)))
                                nil))
         draw-points (fn [x y] (if (or (empty? x) (empty? y))
                               nil
                               (do
                                 (new-path)
                                 (arc (fit-x (first x)) (fit-y (first y)) 3 0 pi-times-2)
                                 (stroke)
                                 (recur (rest x) (rest y)))))
         cx (clojure.java.io/writer width height)
         draw (fn []
                (set-font-size 10)
                (set-color box-color)
                (set-line-width 1)
                                        ; Draw box
                (move-to margin-left margin-top)
                (line-to (- width margin-right) margin-top)
                (line-to (- width margin-right) (- height margin-bottom))
                (line-to margin-left (- height margin-bottom))
                (close-path)
                (stroke)
                                        ; Draw ticks
                (draw-x-ticks (* (int (/ min-x x-tick-step)) x-tick-step))
                (draw-y-ticks (* (int (/ min-y y-tick-step)) y-tick-step))
                                        ; Draw the plots
                (set-line-width 3)
                (run! (fn [p]
                        (let [ [x y color] p ]
                          (set-color color)
                          (draw-points x y))) plots)
                )
         ]
     (with-out cx
       (do
         (draw)
         
         (add-watch (var *window-size*) 0
                    (fn [key ref old-ws ws]
                      (with-out cx
                        (resize)
                        (flush)
                      )))
         
         (add-watch (var *mouse-pos*) 0
                    (fn [key ref old-pos pos]
                      nil
                      ))

         )))))
