(defn linspace
  "Creates an evenly spaced vector from a to b with n points"
  ([a b] (linspace a b 100))
  ([a b n] (let [dx (float (/ (- b a) (- n 1)))]
            (vec (range a (+ b dx) dx)))))

(defn plot
  "Plots a series. Matlab style."
  ([x y] (let [width (*window-size* 0)
               height (int (/ width 3))
               margin-top 50
               margin-bottom 50
               margin-left 50
               margin-right 50
               content-width (- width margin-left margin-right)
               content-height (- height margin-top margin-bottom)
               min-x (apply min x)
               min-y (apply min y)
               max-x (apply max x)
               max-y (apply max y)
               range-x (- max-x min-x)
               range-y (- max-y min-y)
               fit-x (fn [x] (+ (* (/ (- x min-x) range-x) content-width) margin-left))
               fit-y (fn [y] (+ (* (/ (- max-y y) range-y) content-height) margin-top))
               x-tick-step 0.2
               y-tick-step 0.2
               draw-x-ticks (fn [x] (if (< x max-x)
                                      (let [fx (fit-x x)]
                                        (move-to fx margin-top)
                                        (line-to fx (+ margin-top 6))
                                        (stroke)
                                        (move-to fx (- height margin-bottom))
                                        (line-to fx (- height margin-bottom 6))
                                        (stroke)
                                        (move-to fx (- height margin-bottom))
                                        (print x)
                                        (recur (+ x x-tick-step)))
                                      nil))
               draw-y-ticks (fn [y]
                              (if (< y max-y)
                                (let [fy (fit-y y)]
                                  (move-to margin-left fy)
                                  (line-to (+ margin-left 6) fy)
                                  (stroke)
                                  (move-to (- width margin-right) fy)
                                  (line-to (- width margin-right 6) fy)
                                  (stroke)
                                  (move-to 0 fy)
                                  (print y)
                                  (recur (+ y y-tick-step)))
                                nil))
               draw (fn [x y] (if (or (empty? x) (empty? y))
                                nil
                                (do
                                  (line-to (fit-x (first x)) (fit-y (first y)))
                                  (recur (rest x) (rest y)))))
               ]
           (image (with-canvas width height
                    (do
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
                      ; Draw the plot
                      (move-to (fit-x (first x)) (fit-y (first y)))
                      (draw (rest x) (rest y))
                      (stroke)
               ))))))
