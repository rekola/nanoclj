(in-ns 'java.util.UUID)

(defn randomUUID
  "Creates a random UUID"
  [] (let [ng (SecureRandom)
           b (byte-array 16)]
       (.nextBytes ng b)
       (aset b 6 (bit-or (bit-and (aget b 6) 0x0f) 0x40)) ; set to version 4
       (aset b 8 (bit-or (bit-and (aget b 8) 0x3f) 0x80)) ; set to IETF variant
       (UUID b)))

(defn getLeastSignificantBits
  [uuid] (uuid 0))

(defn getMostSignificantBits
  [uuid] (uuid 1))

(defn version
  "Returns the version of the given UUID"
  [uuid] (bit-and (bit-shift-right (uuid 1) 12) 0x0f))

(defn variant
  "Returns the variant of the given UUID. This class uses the Leach-Salz variant (2)"
  [uuid] (let [lsb (uuid 0)]
           (bit-and (unsigned-bit-shift-right lsb (- 64 (unsigned-bit-shift-right lsb 62)))
                    (bit-shift-right lsb 63))))
