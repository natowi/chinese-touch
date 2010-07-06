#!/usr/bin/python
# coding: utf-8

import sys, re, time, copy
from lxml import etree

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

# TODO: multi-language support, by target language
target_lang = "eng"
see_also = "see: "
see_antonym = "antonym: "

# unique list fulltext patterns, referencing unique word id lists
ft_patterns = {}

class Reading:
	def __init__( self, text ):
		self.text = text
		self.kanji_restriction = []

class Sense:
	def __init__( self ):
		self.reset()
	def write( self, word, readings, parent_locals ):
		if not self.glosses.has_key( target_lang ):
			return
		# TODO: cross references to other head words?
		parent_locals = copy.copy( parent_locals )
		definition = ""
		for gloss in self.glosses[ target_lang ]:
			if definition and gloss!="; ":
				definition += ", "
			definition += gloss
		parent_locals["definition"] = definition
		comment = ""
		for c in self.comments:
			if comment:
				comment += "; "
			comment += c
		for c in self.references:
			if comment:
				comment += "; "
			comment += see_also+c
		for c in self.antonyms:
			if comment:
				comment += "; "
			comment += see_antonym+c
		parent_locals["comment"] = comment
		type = ""
		for t in self.types:
			if type:
				type += "; "
			type += t
		parent_locals["type"] = type
		pronunciation = ""
		for r in readings:
			if not r.kanji_restriction or word in r.kanji_restriction:
				if pronunciation:
					pronunciation += ", "
				pronunciation += r.text
		parent_locals["pronunciation"] = pronunciation
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
			if ft_pattern not in words + [""]:
				if ft_patterns.has_key(ft_pattern):
					ft_patterns[ft_pattern][parent_locals["id"]] = True
				else: ft_patterns[ft_pattern] = { parent_locals["id"] : True }
		parent_locals["id"]+=1
		parent_locals["duplicate_id"]+=1
	def reset( self ):
		self.glosses = {}
		self.kanji_restriction = []
		self.reading_restriction = []
		self.types = []
		self.references = []
		self.antonyms = []
		self.comments = []
	def merge( self, sense ):
		for lang in sense.glosses:
			if self.glosses.has_key( lang ):
				self.glosses[lang].append( "; " )
			else:
				self.glosses[lang] = []
			self.glosses[lang] += sense.glosses[lang]
		#self.kanji_restriction += sense.kanji_restriction
		#self.reading_restriction += sense.reading_restriction
		#self.types += sense.types
		self.references += sense.references
		self.antonyms += sense.antonyms
		self.comments += sense.antonyms

context = etree.iterparse( sys.stdin )
for action, elem in context:
	if elem.tag=="entry" and action=="end":
		entry = elem
		words = []
		readings = []
		senses = []
		sense = None
		for child in entry:
			if child.tag=="ent_seq":
				file_offset = int( child.text )
			if child.tag=="k_ele" or child.tag=="r_ele":
				for subchild in child:
					if subchild.tag=="keb" or subchild.tag=="reb":
						words.append( subchild.text )
					if subchild.tag=="reb":
						readings.append( Reading(subchild.text) )
					if subchild.tag=="re_restr":
						readings[-1].kanji_restriction.append( subchild.text )
			if child.tag=="sense":
				sense = Sense()
				senses.append( sense )
				for subchild in child:
					if subchild.tag=="pos":
						sense.types.append( subchild.text )
					if subchild.tag=="xref":
						sense.references.append( subchild.text )
					if subchild.tag=="and":
						sense.antonyms.append( subchild.text )
					if subchild.tag=="misc" or subchild.tag=="s_inf" or subchild.tag=="field":
						sense.comments.append( subchild.text )
					if subchild.tag=="gloss":
						lang_key = "{http://www.w3.org/XML/1998/namespace}lang"
						if subchild.attrib.has_key( lang_key ):
							lang = subchild.attrib[ lang_key ]
						else:
							# When absent, the value "eng" (i.e. English) is 
							# the default value.
							lang = "eng"
						if not sense.glosses.has_key( lang ):
							sense.glosses[ lang ] = [ subchild.text ]
						else:
							sense.glosses[ lang ].append( subchild.text )
		
		for word in words:
			duplicate_id = 0
			global_sense = Sense()
			for sense in senses:
				if sense.comments or sense.references or sense.antonyms \
						or sense.kanji_restriction 	or sense.reading_restriction:
					# write current sense IF restrictions apply:
					if not sense.kanji_restriction or word in sense.kanji_restriction:
						if not sense.reading_restriction:
							sense.reading_restriction = readings
						sense.write( word, sense.reading_restriction, locals() )
				else:
					if sense.types:
						# the part-of-speech of an earlier sense will apply to
						# later senses unless there is a new part-of-speech indicated
						global_sense.write( word, readings, locals() )
						global_sense.reset()
						global_sense.types = sense.types
					# add current sense data global_sense:
					global_sense.merge( sense )
			# write accumulated global sense:
			global_sense.write( word, readings, locals() )
		

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

