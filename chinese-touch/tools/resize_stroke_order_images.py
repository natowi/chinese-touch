#!/usr/bin/python2

import os, re

l=os.listdir(".")
for f in l:
	try:
		char, suffix = re.findall( "(.*)-(.*).png", f )[0]
	except:
		print( "anormal file name: %s" % f )
		continue
	os.popen( "convert %s -resize 50%% stroke-order/u%04x-%s.png" % (f, ord(char.decode("utf-8")[0]), suffix) )

