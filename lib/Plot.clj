(defn linspace
  "Creates an evenly spaced vector from a to b with n points"
  ([a b] (linspace a b 100))
  ([a b n] (let [dx (float (/ (- b a) (- n 1)))]
            (vec (range a (+ b dx) dx)))))

(defn plot
  "Plots a series. Matlab style."
  ([x y] (let [width 500
               height 250
               v-margin 5
               content-height (- height v-margin v-margin)
               min-x (apply min x)
               min-y (apply min y)
               range-x (- (apply max x) min-x)
               range-y (- (apply max y) min-y)
               fit-x (fn [x] (* (/ (- x min-x) range-x) width))
               fit-y (fn [y] (+ (* (/ (- y min-y) range-y) content-height) v-margin))
               draw (fn [x y] (if (or (empty? x) (empty? y))
                                nil
                                (do
                                  (line-to (fit-x (first x)) (fit-y (first y)))
                                  (recur (rest x) (rest y)))))
               ]
           (with-canvas width height
             (do
               (move-to (fit-x (first x)) (fit-y (first y)))
               (draw (rest x) (rest y))
               (stroke)
               )))))
