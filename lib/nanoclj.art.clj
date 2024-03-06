(ns nanoclj.art
  "A namespace for visualization related assets")

(def mk-gradient nanoclj.lang.Gradient)
(def mk-shape nanoclj.lang.Shape)

(def gradient
  "Predefined gradient database. Call with a keyword to get a specific gradient."
  {
   :black-body (mk-gradient "#ff3a00" "#ff6a00" "#ff8808" "#ff9f44" "#ffb267" "#ffc185" "#ffce9e" "#ffd9b5" "#ffe2c9" "#ffeadb" "#fff1ea" "#fff7f8" "#f9f6ff" "#eff0ff" "#e6ebff" "#dfe7ff" "#d9e3ff" "#d3e0ff" "#cfddff" "#cbdaff" "#c7d8ff" "#c4d6ff" "#c1d4ff" "#bed3ff" "#bcd1ff" "#bad0ff" "#b8cfff" "#b6ceff" "#b4ccff" "#b3ccff")
   :orange-blue (mk-gradient "#ffa700" "#fd9101" "#f77b03" "#ee6507" "#e2500c" "#d23d12" "#c02b1a" "#ac1c23" "#97102d" "#810738" "#6b0144" "#550050" "#41025d" "#2f096b" "#1f1279" "#121f86" "#092f94" "#0241a2" "#0055af" "#016bbb" "#0781c7" "#1097d2" "#1cacdc" "#2bc0e5" "#3dd2ed" "#50e2f3" "#65eef8" "#7bf7fc" "#91fdfe" "#a7ffff")
   :mathematica/pastel (mk-gradient "#c378f0" "#c782f1" "#cc8ef3" "#d097ef" "#d69ddc" "#dba3cb" "#e1a9bd" "#e4aeb1" "#e8b3a5" "#ebb9a0" "#eec09c" "#f0c798" "#f2ce97" "#f4d696" "#f4dc96" "#f5e497" "#f6ea98" "#f4ee9d" "#f3f2a0" "#eff3a7" "#eaf2b0" "#e4f1b9" "#d9ecc5" "#cde7d2" "#bfe0de" "#afd8e7" "#9ccff2" "#8dc7f2" "#7cbdef" "#6eb5ed")
   :mathematica/alpine-colors (mk-gradient "#475a7b" "#485f7a" "#4a6579" "#4c6978" "#4e6e75" "#50706f" "#537468" "#557762" "#58795f" "#5d7d5d" "#617f5b" "#66835b" "#6c865f" "#748a65" "#7a8d6a" "#839271" "#8a9678" "#939b81" "#9b9f88" "#a4a691" "#acae99" "#b6b5a2" "#bebdaa" "#c8c6b3" "#d1d0bd" "#dad9c5" "#e4e3cf" "#ecebd7" "#f7f6e2" "#ffffea")
   :mathematica/candy-colors (mk-gradient "#673457" "#723558" "#803659" "#8b375a" "#98395c" "#a33c5f" "#ae4064" "#b94367" "#c14a6e" "#c85376" "#ce5c7f" "#d46587" "#d46f91" "#d47a9b" "#d482a4" "#d28cae" "#ce93b5" "#c89cbe" "#c4a2c6" "#bfa9cc" "#baafd2" "#b5b6d8" "#b1bcdd" "#b0c4e0" "#aecbe4" "#add2e7" "#abd6e7" "#aad9e5" "#a9dce3" "#a8dfe1")
   :mathematica/coffee-tones (mk-gradient "#685447" "#705b4b" "#7a6450" "#826a54" "#8c7259" "#957a5d" "#9e8160" "#a3835e" "#a7855c" "#ac8859" "#b18a57" "#b68d54" "#ba9053" "#be9555" "#c29957" "#c79e5a" "#caa15b" "#cfa65e" "#d2ac65" "#d7b470" "#dbbc7b" "#e0c486" "#e4cb91" "#e8d39c" "#ebdbac" "#eee2bc" "#f1eacf" "#f4f0de" "#f7f9f1" "#f9ffff")
   :mathematica/fruit-punch-colors (mk-gradient "#ff7f00" "#ff8500" "#fe8d00" "#fe9300" "#fd9900" "#f89e02" "#f2a204" "#eda606" "#e7a908" "#e2ab0f" "#dcaa17" "#d6a91f" "#d2a825" "#cda531" "#cba03c" "#c89a4b" "#c69556" "#c58e65" "#c78772" "#c97e81" "#ca768e" "#cd6f9a" "#d268a1" "#d662a9" "#dc5bb0" "#e057b4" "#e658a9" "#ea59a0" "#f05b93" "#f55c8a")
   :mathematica/thermometer-colors (mk-gradient "#291ecb" "#312fd3" "#3b43de" "#4456e5" "#556eea" "#6383ee" "#7398f1" "#83aaf2" "#93bcf3" "#a2c8f1" "#afd3ef" "#bdddea" "#c7e1e5" "#d3e6df" "#d9e5d7" "#e0e2cc" "#e5e0c4" "#e6d6b7" "#e7ceac" "#e6c19f" "#e3b393" "#dfa486" "#d89179" "#d17f6d" "#c86b60" "#be5854" "#b24347" "#a5333e" "#952333" "#88152a")
   })

(def shapes
  "Predefined shape database. Call with a keyword to get a specific shape."
  {
   })

(defn plot-gradient
  [g] (let [[cell-width h] *cell-size*
            w (* 10 cell-width)
            cx (clojure.java.io/writer w h :rgb)]
        (image (with-out cx
                 (move-to 0 0)
                 (line-to w 0)
                 (line-to w h)
                 (line-to 0 h)
                 (close-path)
                 (set-color g [ 0 0 ] [ w 0 ])
                 (fill-preserve)
                 (set-line-width 1)
                 (set-color (if (= *theme* :dark)
                              [ 1 1 1 0.5 ]
                              [ 0 0 0 0.5 ]))
                 (stroke)
                 ))))

(defn plot-shape
  [shape width height] (image (with-out (clojure.java.io/writer width height :rgb)
                                (fill shape)
                                )))
