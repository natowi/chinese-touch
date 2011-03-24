#!/usr/bin/python2
# coding: utf-8

import sys, os, re

try:
	base_dir = sys.argv[1]
except IndexError:
	base_dir = "."
data_dir = os.path.join( base_dir, "cjklib", "data" )
fn_char_decomp = os.path.join( data_dir, "characterdecomposition.csv" )
fn_radical_char_map = os.path.join( data_dir, "radicalequivalentcharacter.csv" )


radical_char_map = {}
f_radical_char_map = open( fn_radical_char_map )
for line in f_radical_char_map:
	if line[0]=="#":
		continue
	radical, char, locale = line.split(",")
	radical = eval( radical )
	char = eval( char )
	# FIXME: only valid for PCR chinese:
	if "C" in locale:
		radical_char_map[ radical ] = char
f_radical_char_map.close()

ignore_chars = "⿰⿱⿲⿳⿴⿵⿶⿷⿸⿹⿺⿻[]0123456789？"
char_decomp = {}
f_char_decomp = open( fn_char_decomp )
for line in f_char_decomp:
	if line[0]=="#":
		continue
	# Flags:
	# O: Checked, which means the entry has been through a quick check
	# G: Origin from "Groovy set"
	# S: The character variant described is only found as a subpart in another character
	char, decomposition, glyph, subindex, flags = line.split(",")
	char = eval( char )
	decomposition = eval( decomposition )
	if not char_decomp.has_key( char ):
		char_decomp[ char ] = []
	for component in decomposition.decode("utf-8"):
		component = component.encode("utf-8")
		if component in ignore_chars:
			continue
		if component not in char_decomp[ char ]:
			if radical_char_map.has_key( component ):
				component = radical_char_map[ component ]
			char_decomp[ char ].append( component )
f_char_decomp.close()

print( "CREATE TABLE components (id INTEGER PRIMARY KEY, character TEXT, component TEXT);" )
for char in char_decomp:
	for component in char_decomp[ char ]:
		print( "insert into components (character, component) values ('%s','%s');" % (char, component) )
print( "CREATE INDEX i_components_id ON components(id);" )
print( "CREATE INDEX i_components_character ON components(character ASC);" )
print( "CREATE INDEX i_components_component ON components(component ASC);" )

