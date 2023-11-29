# Hacking

## Vectors

Vectors support O(1) conjoin by having an padding in the backing
tensor after the data values, where new values can be added. Since all
vector cells know their size, new values can be added in the shared
tensor without affecting other vectors that share the same tensor. Of
course, only one vector can use the extra space for growing, and if
the space is used, a copy needs to be made.

## Hash maps and sets

Hash maps and sets support O(1) conjoin by having a shared hash table
represented by a tensor. Each hash entry has a key and order number
(and a value, in the case of a map), and the hash maps and hash sets
disregard those hash entries whose order number is larger than their
size.