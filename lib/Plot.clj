(def ^:private plot-colors '([0 0.4470 0.7410]
                             [0.8500 0.3250 0.0980]
                             [0.9290 0.6940 0.1250]
                             [0.4940 0.1840 0.5560]
                             [0.4660 0.6740 0.1880]
                             [0.3010 0.7450 0.9330]
                             [0.6350 0.0780 0.1840]
                             ))

(def-macro (with-out new-out $ body)
  `(let ((prev-out *out*)
         (tmp ,new-out))
     (set! *out* tmp)
     ,@body
     (set! *out* prev-out)
     tmp))

(defn linspace
  "Creates an evenly spaced vector from a to b with n points"
  ([a b] (linspace a b 100))
  ([a b n] (loop [a a n (double n) acc []]
             (if (< n 0) acc
                 (recur (+ a (/ (- b a) n)) (dec n) (conj acc a))))))

(defn geo-plot
  "Plots a map"
  ([& args]
   (let [width (*window-size* 0)
         height (int (/ width 3))
         ]
     )))

(defn plot
  "Plots a series. Matlab style."
  ([& args]
   (let [width (*window-size* 0)
         height (int (/ width 3))
         box-color [ 0.0 0.0 0.0 ]
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
         draw-line (fn [x y] (if (or (empty? x) (empty? y))
                               nil
                               (do
                                 (line-to (fit-x (first x)) (fit-y (first y)))
                                 (recur (rest x) (rest y)))))
         cx (clojure.java.io/writer width height [ 1 1 1 ])
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
                          (move-to (fit-x (first x)) (fit-y (first y)))
                          (draw-line (rest x) (rest y))
                          (stroke))) plots)
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
         box-color [ 0.0 0.0 0.0 ]
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
         cx (clojure.java.io/writer width height [ 1 1 1 ])
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
