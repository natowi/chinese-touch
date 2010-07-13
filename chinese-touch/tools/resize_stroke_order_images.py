#!/usr/bin/python

import os

l=os.listdir(".")
for f in l:
	os.popen( "convert %s -resize 50%% stroke-order/u%04x.png" % (f, ord(f.decode("utf-8")[0])) )

