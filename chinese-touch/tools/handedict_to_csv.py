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
type="-"
pronunciation="-"
definition="-"
comment=""
rating="0"
file_id="0"
file_offset=0
for line in sys.stdin:
	line = line.replace( "\t", " " );
	line = line.replace( "(u.E.)", "" );
	line = line.replace( "&gt", ">" );
	#                       trad.   simpl.    pinyin      definition
	results = re.findall( "^([^ ]*) ([^ ]*) \[([^\]]*)\] /(.*/)", line )
	result = results[0]
	traditional = result[0]
	word = result[1]
	pronunciation = translate_pinyin( result[2] )
	raw_definition = result[3]
	fixed_definition = ""
	parenthesis_level=0
	# try to fix slashes in parenthesis:
	for c in raw_definition:
		if c in "(<[{":
			parenthesis_level+=1
		elif c in ")>]}":
			parenthesis_level-=1
		
		if c in "/" and parenthesis_level>0:
			fixed_definition += ","
		else:
			fixed_definition += c
	# ignore slash-fix on irregular parenthesis expressions:
	if parenthesis_level!=0:
		fixed_definition = raw_definition
	#                      transl.    comment        type                example
	results = re.findall( "(.*?) *(?:<(.*?)>)? *(?:\(([^\(]*)\))? *(?:; *(Bsp\.:[^/]*))? *[/]", fixed_definition )
	test=False
	for duplicate_id in xrange(len(results)):
		test=True
		result = results[ duplicate_id ]
		definition = result[0]
		comment = result[1]
		type = result[2]
		if( len(comment) and len(result[3]) ):
			comment += "; "
		comment += result[3]
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
	if( not test ):
		print line
		raise Exception("Parse error")
	file_offset+=1

if "--inserts" in sys.argv[1:]:
	print "CREATE INDEX i_words_word ON words (word);"
	print "COMMIT;"

