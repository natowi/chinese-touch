#!/usr/bin/python2

import os, re

l=os.listdir(".")
for f in l:
	try:
		char, suffix = re.findall( "(.*)-(.*).png", f )[0]
	except:
		print( "anormal file name: %s" % f )
		continue
	dest_file_name = "stroke-order/u%04x-%s.png" % ( ord(char.decode("utf-8")[0]), suffix )
	try:
		os.stat( dest_file_name )
	except OSError:
		os.popen( "convert %s -fill '#ffffff00' -opaque none -alpha opaque -resize 50%% %s" % (f, dest_file_name) )

