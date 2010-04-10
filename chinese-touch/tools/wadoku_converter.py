#!/usr/bin/python
# coding: utf-8

import sys, re, time, copy


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

ft_col_names = [
	"pronunciation",
	"definition",
	"comment"
]

col_names.remove("id")
print "BEGIN TRANSACTION;"
print "CREATE TABLE words (atime NUMERIC, id INTEGER PRIMARY KEY, word TEXT, lesson_id NUMERIC, duplicate_id NUMERIC, type TEXT, pronunciation TEXT, definition TEXT, comment TEXT, rating NUMERIC, file_id NUMERIC default 0, file_offset NUMERIC default 0);"

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

# unique list fulltext patterns, referencing unique word id lists
ft_patterns = {}

previous_line = ""
for line in sys.stdin:
	line = previous_line+" "+line.strip()
	if len(line) and line[-1]!="/":
		previous_line = line
		continue
	previous_line = ""
	line += "\n"
	line = line.replace( "\t", " " );
	line = line.replace( "(u.E.)", "" );
	line = line.replace( "&gt", ">" );
	line = line.replace( "http://", "" );
	#print line
	#                       word(s) kana       sent.-cmt       gen.-cmt    definition
	results = re.findall( "^(.*?) \[([^\]]*)\] ((?:\[\].*?/ )*)([^/]*[^ ])?/(.*/)", line )
	try:
		result = results[0]
	except:
		sys.stderr.write( line )
		file_offset+=1
		continue
	raw_words = re.split( "[;,. ]", result[0] )
	pronunciation = result[1]
	sentence_comment = result[2]
	sentence_comment = sentence_comment.replace( "[]", "" )
	sentence_comment = sentence_comment.replace( "/ ", ", " )
	sentence_comment = sentence_comment.replace( "/", ": " )
	sentence_comment = sentence_comment[:-2]
	generic_comment = result[3]
	if len(sentence_comment) and len(generic_comment):
		sentence_comment += "; "
	generic_comment = sentence_comment + generic_comment
	raw_definition = result[4]
	words = []
	for word in raw_words:
		word = word.strip()
		# parse words with seperators as comments:
		if( "･" in word and "･"!=word ):
			if len(generic_comment):
				generic_comment += "; "
			generic_comment += word
			real_word = word.replace("･","")
			if( real_word not in raw_words ):
				words.append( real_word )
		# append real words to the current word list
		elif( len(word) ):
			words.append( word )
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
	results = re.findall( "(.*?) *(?:\((.*?)\))? *[/]", fixed_definition )
	duplicate_id = 0
	for word in words:
		duplicate_id = 0
		definition = ""
		comment = generic_comment
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
					ft_pattern_list += re.split( "[ ,;:.\!\?\-/\(\)\[\]\{\}\<\>0-9'\"]", value )
				value = value.replace( "'", "''" ).strip()
				insert += "'"+value+"'"
			insert += ");"
			print insert
			for ft_pattern in ft_pattern_list:
				ft_pattern = extended_lower( ft_pattern )
				if ft_pattern!="":
					if ft_patterns.has_key(ft_pattern):
						ft_patterns[ft_pattern][id] = True
					else: ft_patterns[ft_pattern] = { id : True }
			id+=1
			duplicate_id+=1
		for di in xrange(len(results)):
			result = results[ di ]
			part_definition = result[0]
			part_comment = result[1]
			if len(part_comment):
				if len(comment):
					part_comment = comment+"; "+part_comment
				write_row( part_definition, part_comment, locals() )
			else:
				if len(definition):
					definition += "; "
				definition += part_definition
		if len(definition) or not duplicate_id:
			write_row( definition, comment, locals() )
	if( duplicate_id==0 ):
		sys.stderr.write(line)
	file_offset+=1

print "CREATE INDEX i_words_word ON words (word);"

print "CREATE TABLE ft_patterns (pattern TEXT, id PRIMARY_KEY);"
print "CREATE TABLE ft_matches (pattern_id NUMERIC, word_id NUMERIC);"
pattern_id = 0
for ft_pattern in ft_patterns:
	# ignore very frequent patterns
	if len( ft_patterns[ft_pattern] ) > 1000:
		continue
	print "INSERT INTO ft_patterns (pattern,id) values ('"+ft_pattern+"',"+str(pattern_id)+");"
	for word_id in ft_patterns[ft_pattern]:
		print "INSERT INTO ft_matches (pattern_id,word_id) values ("+str(pattern_id)+","+str(word_id)+");"
	pattern_id += 1
print "CREATE INDEX i_ft_patterns_pattern ON ft_patterns (pattern);"
print "CREATE INDEX i_ft_matches_pattern_id ON ft_matches (pattern_id);"
print "CREATE INDEX i_ft_matches_word_id ON ft_matches (word_id);"
print "COMMIT;"

