#!/usr/bin/python
# coding: utf-8

import sys, re, time, copy

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
	"r5" : "r",
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
	"u:1" : "ǖ",
	"u:2" : "ǘ",
	"u:3" : "ǚ",
	"u:4" : "ǜ",
	"u:5" : "ü",
}

untone_mapping = {}
for key in first_mapping:
	if key[-1] != "5":
		untone_mapping[ first_mapping[key] ] = key[0]
for key in second_mapping:
	if key[-1] != "5":
		untone_mapping[ second_mapping[key] ] = key[0]

def translate_pinyin( raw_pinyin ):
	syllables = raw_pinyin.split(" ")
	new_syllables = []
	for _syl in syllables:
		lowercased = False
		if( _syl != _syl.lower() ):
			_syl = _syl.lower()
			lowercased = True
		try:
			syl, tone = _syl[:-1].decode("utf-8"), int(_syl[-1:])
		except:
			new_syllables.append( _syl )
			continue
		
		if syl=="r" and tone==5 and len(new_syllables):
			new_syllables[-1] += syl
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
		
		if lowercased:
			new_syl = new_syl[0].upper()+new_syl[1:]
		
		new_syllables.append( new_syl )
		#print _syl + " -> " + new_syl
	return " ".join(new_syllables)


extended_lower_map = {
	"Ä" : "ä",
	"Ö" : "ö",
	"Ü" : "ü",
	"Ā" : "ā",
	"Á" : "á",
	"Ǎ" : "ǎ",
	"À" : "à",
	"Ē" : "ē",
	"É" : "é",
	"Ě" : "ě",
	"È" : "è",
	"Ō" : "ō",
	"Ó" : "ó",
	"Ǒ" : "ǒ",
	"Ò" : "ò",
	"Ī" : "ī",
	"Í" : "í",
	"Ǐ" : "ǐ",
	"Ì" : "ì",
	"Ū" : "ū",
	"Ú" : "ú",
	"Ǔ" : "ǔ",
	"Ù" : "ù",
	"Ǖ" : "ǖ",
	"Ǘ" : "ǘ",
	"Ǚ" : "ǚ",
	"Ǜ" : "ǜ",
}

def extended_lower( s ):
	s = s.lower()
	result = ""
	for c in s.decode("utf-8"):
		c = c.encode("utf-8")
		if extended_lower_map.has_key(c):
			c = extended_lower_map[c]
		result += c
	return result

# UNIHAN Range:
#   CJK Unified Ideographs Extension A: Range: 3400-4DBF
#   CJK Unified Ideographs: Range: 4E00-9FAF
# Dictionary Radicals:
#   Kangxi Radicals: Range 2F00-2FDF 
def filter_pattern( s ):
	result = ""
	for c in s.decode("utf-8"):
		if 0x3400<=ord(c)<=0x4DBF or 0x4E00<=ord(c)<=0x9FAF:
			continue
		else:
			result += c.encode("utf-8")
	return result


col_names = [ 
	"atime",
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

ft_col_names = [
	"pronunciation",
	"definition",
	"comment"
]

print "BEGIN TRANSACTION;"
print "CREATE TABLE words (atime NUMERIC, id INTEGER PRIMARY KEY, word TEXT, lesson_id NUMERIC, duplicate_id NUMERIC, type TEXT, pronunciation TEXT, definition TEXT, comment TEXT, rating NUMERIC, file_id NUMERIC default 0, file_offset NUMERIC default 0);"

sys.stdin.readline()
atime=int(time.time())
id=1
word="-"
lesson_id=0
type=""
pronunciation="-"
definition="-"
comment=""
rating="0"
file_id="0"
file_offset=0

# unique list fulltext patterns, referencing unique word id lists
ft_patterns = {}

for line in sys.stdin:
	if len(line)==0 or line[0]=="#":
		continue
	line = line.replace( "\t", " " );
	line = line.replace( "(u.E.)", "" );
	line = line.replace( "&gt", ">" );
	#                       trad.   simpl.    pinyin      definition
	results = re.findall( "^([^ ]*) ([^ ]*) \[([^\]]*)\] /(.*/)", line )
	result = results[0]
	words = { "trad" : result[0], "simp" : result[1] }
	pronunciation = translate_pinyin( result[2] )
	raw_definition = result[3]
	inline_pinyin = re.findall( "\[([^\]]*)\]", raw_definition )
	for pinyin in inline_pinyin:
		raw_definition = raw_definition.replace( pinyin, translate_pinyin(pinyin) )
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
	#                      transl.    comment
	results = re.findall( "(.*?) *(?:\(([^\(]*)\))? *[/]", fixed_definition )
	test=False
	variants = []
	if "--simplified" in sys.argv or "-s" in sys.argv:
		variants.append( "simp" )
	if "--traditional" in sys.argv or "-t" in sys.argv:
		variants.append( "trad" )
	if len(variants)==0:
		variants = [ "trad", "simp" ]
	duplicate_id = 0
	for variant in variants:
		duplicate_id = 0
		definition = ""
		comment = ""
		if words["simp"] != words["trad"]:
			if variant=="trad":
				comment += "simpl: " + words["simp"]
			elif variant=="simp":
				comment += "trad: " + words["trad"]
		if ("simp" in variants) and ("trad" in variants) \
			and variant=="simp" and words["simp"]==words["trad"]:
				break # only one entry if trad==simpl!
		def write_row( definition, comment, parent_locals ):
			global id, duplicate_id
			parent_locals = copy.copy( parent_locals )
			parent_locals["definition"] = definition
			parent_locals["comment"] = comment
			insert = "insert into words ("+",".join(col_names)+") values ("
			ft_pattern_list = []
			for i in xrange(len(col_names)):
				if( i ): insert+=","
				value = ("%("+col_names[i]+")s") % parent_locals
				if( col_names[i] in ft_col_names ):
					ft_pattern_list += re.split( "[ ,;:.\!\?\-/\(\)\[\]\{\}\<\>0-9'\"\|]", value )
				value = value.replace( "'", "''" ).strip()
				insert += "'"+value+"'"
			insert += ");"
			print insert
			for ft_pattern in ft_pattern_list:
				ft_pattern = extended_lower( ft_pattern )
				ft_pattern = filter_pattern( ft_pattern )
				# do not generate unnecessary self and bogus references:
				if ft_pattern not in words.values() + [""]:
					if ft_patterns.has_key(ft_pattern):
						ft_patterns[ft_pattern][id] = True
					else: ft_patterns[ft_pattern] = { id : True }
			id+=1
			duplicate_id+=1
		for di in xrange(len(results)):
			test=True
			word = words[variant]
			result = results[ di ]
			part_definition = result[0]
			part_comment = result[1]
			if len(part_comment):
				if len(comment):
					part_comment = part_comment+"; "+comment
				write_row( part_definition, part_comment, locals() )
			else:
				if len(definition):
					definition += "; "
				definition += part_definition
		if len(definition) or not duplicate_id:
			write_row( definition, comment, locals() )
	if( not test ):
		print line
		raise Exception("Parse error")
	file_offset+=1

print "CREATE INDEX i_words_word ON words (word);"

print "CREATE TABLE ft_patterns (pattern TEXT, id PRIMARY_KEY);"
print "CREATE TABLE ft_matches (pattern_id NUMERIC, word_id NUMERIC);"
pattern_id = 0
for ft_pattern in ft_patterns:
	## ignore very frequent patterns
	#if len( ft_patterns[ft_pattern] ) > 1000:
	#	continue
	print "INSERT INTO ft_patterns (pattern,id) values ('"+ft_pattern+"',"+str(pattern_id)+");"
	for word_id in ft_patterns[ft_pattern]:
		print "INSERT INTO ft_matches (pattern_id,word_id) values ("+str(pattern_id)+","+str(word_id)+");"
	pattern_id += 1
print "CREATE INDEX i_ft_patterns_pattern ON ft_patterns (pattern);"
print "CREATE INDEX i_ft_matches_pattern_id ON ft_matches (pattern_id);"
print "CREATE INDEX i_ft_matches_word_id ON ft_matches (word_id);"
print "COMMIT;"

