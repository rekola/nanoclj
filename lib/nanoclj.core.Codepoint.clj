(in-ns 'nanoclj.core.Codepoint)

(def UNASSIGNED 0)
(def UPPERCASE_LETTER 1)
(def LOWERCASE_LETTER 2)
(def TITLECASE_LETTER 3)
(def MODIFIER_LETTER 4)
(def OTHER_LETTER 5)
(def NON_SPACING_MARK 6)
(def COMBINING_SPACING_MARK 7)
(def ENCLOSING_MARK 8)
(def DECIMAL_DIGIT_NUMBER 9)
(def LETTER_NUMBER 10)
(def OTHER_NUMBER 11)
(def CONNECTOR_PUNCTUATION 12)
(def DASH_PUNCTUATION 13)
(def START_PUNCTUATION 14)
(def END_PUNCTUATION 15)
(def INITIAL_QUOTE_PUNCTUATION 16)
(def FINAL_QUOTE_PUNCTUATION 17)
(def OTHER_PUNCTUATION 18)
(def MATH_SYMBOL 19)
(def CURRENCY_SYMBOL 20)
(def MODIFIER_SYMBOL 21)
(def OTHER_SYMBOL 22)
(def SPACE_SEPARATOR 23)
(def LINE_SEPARATOR 24)
(def PARAGRAPH_SEPARATOR 25)
(def CONTROL 26)
(def FORMAT 27)
(def SURROGATE 28)
(def PRIVATE_USE 29)

(def MIN_CODE_POINT 0)
(def MAX_CODE_POINT 0x10ffff)

(def MIN_SUPPLEMENTARY_CODE_POINT 0x100000)

(defn getType
  "Returns the type of the Unicode character"
  [c] (-category c))

(defn isAlphabetic
  "Returns true if codepoint is alphabetic"
  [c] (let [cat (getType c)] (case cat
                               UPPERCASE_LETTER true
                               LOWERCASE_LETTER true
                               TITLECASE_LETTER true
                               MODIFIER_LETTER true
                               OTHER_LETTER true
                               LETTER_NUMBER true
                               false)))

(defn isLetter
  "Returns true if codepoint is a letter"
  [c] (let [cat (getType c)] (case cat
                               UPPERCASE_LETTER true
                               LOWERCASE_LETTER true
                               TITLECASE_LETTER true
                               MODIFIER_LETTER true
                               OTHER_LETTER true
                               false)))

(defn isLetterOrDigit
  "Returns true if codepoint is a letter"
  [c] (let [cat (getType c)] (case cat
                               UPPERCASE_LETTER true
                               LOWERCASE_LETTER true
                               TITLECASE_LETTER true
                               DECIMAL_DIGIT_NUMBER true
                               false)))

(defn isSpaceChar
  "Returns true if codepoint is a space charater"
  [c] (let [cat (getType c)] (case cat
                               SPACE_SEPARATOR true
                               LINE_SEPARATOR true
                               PARAGRAPH_SEPARATOR true
                               false)))

(defn isWhitespace
  "Returns true if codepoint is whitespace"
  [c] (let [cat (getType c)] (case cat
                               SPACE_SEPARATOR true
                               LINE_SEPARATOR true
                               PARAGRAPH_SEPARATOR true
                               CONTROL true
                               false)))

(defn isDigit
  "returns true if codepoint is a digit"
  [c] (= (getType c) DECIMAL_DIGIT_NUMBER))

(defn isISOControl
  "returns true if codepoint is a digit"
  [c] (= (getType c) CONTROL))

(defn isLowerCase
  "Returns true if codepoint is in lower case"
  [c] (= (getType c) LOWERCASE_LETTER))

(defn isUpperCase
  "Returns true if codepoint is in upper case"
  [c] (= (getType c) UPPERCASE_LETTER))

(defn isTitleCase
  "Returns true if codepoint is in title case"
  [c] (= (getType c) TITLECASE_LETTER))

(defn isDefined
  "Returns true if the codepoint has been assigned a meaning in Unicode"
  [c] (not= (getType c) UNASSIGNED))

(defn isValidCodePoint
  "Returns true if codepoint is valid"
  [c] (and (>= 0 MIN_CODE_POINT) (<= c MAX_CODE_POINT)))

(defn isSupplementaryCodePoint
  "Returns true if codepoint is suplementary"
  [c] (>= c MIN_SUPPLEMENTARY_CODE_POINT))

(defn isBmpCodePoint
  "Returns true if the codepoint belongs to the Basic Multilingual Plane"
  [c] (and (>= 0 c) (<= c 0xffff)))

(defn toTitleCase
  "Returns the codepoint in title case"
  [c] (-totitle c))

(defn toUpperCase
  "Returns the codepoint in upper case"
  [c] (-toupper c))

(defn toLowerCase
  "Returns the codepoint in lower case"
  [c] (-tolower c))
