#!/usr/bin/python

import sys, re

def split_variants():
	for line in sys.stdin:
		jp, en = line.split( " - ", 1 )
		jps = jp.split( " " )
		for jp in jps:
			sys.stdout.write( "%s - %s" % (jp, en) )

def convert():
	for line in sys.stdin:
		jp = re.findall( "^(.*?) - ", line )
		en = re.findall( " - ([^({\n]*)", line )
		pr = re.findall( "\((.*?)\)", line )
		cm = re.findall( "{(.*?)}", line )
		#print jp, en, pr, cm
		print( "|-" )
		try:
			print( "| %s" % jp[0] )
		except:
			print( "| " )
		try:
			print( "| %s" % pr[0] )
		except:
			print( "| " )
		try:
			print( "| %s" % en[0] )
		except:
			print( "| " )
		print( "| " ) # word type undefined
		try:
			print( "| %s" % cm[0] )
		except:
			print( "| " )
		print

locals()[sys.argv[1]]()
