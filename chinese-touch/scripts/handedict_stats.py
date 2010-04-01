#!/usr/bin/python
# coding: utf-8

import sys, re, time

sys.stdin.readline()
words_per_character = {}
for line in sys.stdin:
	result = re.findall( "^([^ ]*) ([^ ]*) \[([^\]]*)\] /(.*)/", line )
	result = result[0]
	word = result[1]
	for c in word.decode("utf-8"):
		c = c.encode("utf-8")
		if( words_per_character.has_key(c) ):
			words_per_character[c][0] += 1
		else:
			words_per_character[c] = [1,c]
	continue

print( "characters: %d" % len(words_per_character) )
word_count_frequency = {}
pairs = words_per_character.values()
pairs.sort() # order by char count
# print highest char counts and corresponding characters first:
for pair in pairs[::-1]:
	word_count = pair[0]
	char = pair[1]
	print char,"erscheint in",word_count,"Wörtern"
	if word_count_frequency.has_key( word_count ):
		word_count_frequency[ word_count ][1] += 1
	else: 
		word_count_frequency[ word_count ] = [word_count,1]

print
pairs = word_count_frequency.values()
pairs.sort() # order by char count
# print highest char counts and corresponding count frequencies first:
for pair in pairs[::-1]:
	frequency = pair[1]
	word_count = pair[0]
	print frequency,"verschiedene Zeichen erscheinen in jeweils",word_count,"Wörtern"
