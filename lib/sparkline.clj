(def sparkline (package

(defn bars
  "Draws a sparkline"
  [seq] (if (empty? seq) ""
            (let [max-value (double (apply max seq))]
              (loop [seq seq data []]
                (if (empty? seq) (apply str data)
                    (let [c (case (int (* 8 (/ (max 0 (first seq)) max-value)))
                              0 \space
                              1 \u2581
                              2 \u2582
                              3 \u2583
                              4 \u2584
                              5 \u2585
                              6 \u2586
                              7 \u2587
                              8 \u2588)]
                      (recur (rest seq) (conj data c))))))))
                  
                  
          
))
