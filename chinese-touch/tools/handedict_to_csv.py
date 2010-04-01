#!/usr/bin/python
# coding: utf-8

import sys, re, time

first_mapping = {
	"a1" : "ā",
	"a2" : "á",
	"a3" : "ǎ",
	"a4" : "à",
	"a5" : "a",
	"e1" : "ē",
	"e2" : "é",
	"e3" : "ě",
	"e4" : "è",
	"e5" : "e",
	"o1" : "ō",
	"o2" : "ó",
	"o3" : "ǒ",
	"o4" : "ò",
	"o5" : "o",
}

second_mapping = {
	"i1" : "ī",
	"i2" : "í",
	"i3" : "ǐ",
	"i4" : "ì",
	"i5" : "i",
	"u1" : "ū",
	"u2" : "ú",
	"u3" : "ǔ",
	"u4" : "ù",
	"u5" : "u",
	"ü1" : "ǖ",
	"ü2" : "ǘ",
	"ü3" : "ǚ",
	"ü4" : "ǜ",
	"ü5" : "ü",
}

def translate_pinyin( raw_pinyin ):
	syllables = raw_pinyin.split(" ")
	new_syllables = []
	for _syl in syllables:
		try:
			syl, tone = _syl[:-1].decode("utf-8"), int(_syl[-1:])
		except:
			new_syllables.append( _syl )
			continue
		
		new_syl = ""
		for i in xrange(len(syl)):
			key = syl[i]+str(tone)
			key = key.encode("utf-8")
			#print key
			if( first_mapping.has_key(key) ):
				new_syl += first_mapping[key] + syl[i+1:]
				break
			else:
				new_syl += syl[i]
		if new_syl==syl and tone!=5:
			new_syl=""
			for i in xrange(len(syl)-1,-1,-1):
				key = syl[i]+str(tone)
				key = key.encode("utf-8")
				#print key
				if( second_mapping.has_key(key) ):
					new_syl += second_mapping[key] + syl[i-1::-1]
					break
				else:
					new_syl += syl[i]
			new_syl = new_syl[::-1]
		if new_syl==syl and tone!=5:
			new_syl = _syl
		
		new_syllables.append( new_syl )
		#print _syl + " -> " + new_syl
	return " ".join(new_syllables)

col_names = [ 
	"atime",
	"id",
	"word",
	"lesson_id",
	"duplicate_id",
	"type",
	"pronunciation",
	"definition",
	"comment",
	"rating",
	"file_id",
	"file_offset"
]

if "--inserts" in sys.argv[1:]:
	col_names.remove("id")
	print "BEGIN TRANSACTION;"
	print "CREATE TABLE words (atime NUMERIC, id INTEGER PRIMARY KEY, word TEXT, lesson_id NUMERIC, duplicate_id NUMERIC, type TEXT, pronunciation TEXT, definition TEXT, comment TEXT, rating NUMERIC, file_id NUMERIC default 0, file_offset NUMERIC default 0);"
else:
	for i in xrange(len(col_names)):
		if( i ): sys.stdout.write( "\t" )
		sys.stdout.write( col_names[i] )
	print("")

sys.stdin.readline()
atime=int(time.time())
id=1
word="-"
lesson_id=0
duplicate_id=0
type="-"
pronunciation="-"
definition="-"
comment=""
rating="0"
file_id="0"
file_offset="0"
for line in sys.stdin:
	line = line.replace( "\t", " " );
	line = line.replace( "(u.E.)", "" );
	result = re.findall( "^([^ ]*) ([^ ]*) \[([^\]]*)\] /(.*)/", line )
	result = result[0]
	traditional = result[0]
	word = result[1]
	pronunciation = translate_pinyin( result[2] )
	definitions = result[3].split("/")
	for definition in definitions:
		result = re.findall( "^(.*)\((.*)\)", definition )
		if( len(result) ):
			result = result[0]
			definition = result[0]
			type = result[1]
		if "--inserts" in sys.argv[1:]:
			insert = "insert into words ("+",".join(col_names)+") values ("
			for i in xrange(len(col_names)):
				if( i ): insert+=","
				value = ("%("+col_names[i]+")s") % locals()
				value = value.replace( "'", "''" ).strip()
				insert += "'"+value+"'"
			insert += ");"
			print insert
		else:
			for i in xrange(len(col_names)):
				if( i ): sys.stdout.write( "\t" )
				sys.stdout.write( ("%("+col_names[i]+")s") % locals() )
			print("")
		id+=1

if "--inserts" in sys.argv[1:]:
	print "CREATE INDEX i_words_word ON words (word);"
	print "COMMIT;"

