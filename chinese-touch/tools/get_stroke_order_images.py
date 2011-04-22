#!/usr/bin/python2
# coding: utf-8

import urllib2, re, sys

base_url = "http://commons.wikimedia.org"
try:
	char_list_url = sys.argv[1]
except IndexError:
	char_list_url = "/wiki/Category:Bw.png_stroke_order_images"
headers = { "User-Agent":"Mozilla/5.0 (X11; U; Linux i686) Gecko/20071127 Firefox/2.0.0.11" }

while char_list_url:
	if char_list_url[:5] != "http:":
		url = base_url+char_list_url
	else:
		url = char_list_url
	url = url.replace( "&amp;", "&" )
	print url
	r = urllib2.Request(url, headers=headers)
	char_list = urllib2.urlopen( r ).read()
	char_list_url = re.findall( """<a href="([^"]*)" title="Category:[^"]*.png stroke order images">next [0-9]*</a>""", char_list )
	if char_list_url:
		char_list_url = char_list_url[0]
	else:
		char_list_url = None
		
	# create unified name-to-url-mapping:
	meta_url_dict = {}
	meta_urls = re.findall( """<a href="([^"]*\\.png)" title="File:([^"]*\\.png)">""", char_list )
	if len(meta_urls):
		for x in meta_urls:
			meta_url_dict[ x[1] ] = x[0]
	else:
		# fallback to link format in progress pages:
		meta_urls = re.findall( """<a href="([^"]*\\.png)" class="image" title="([^"]*)">""", char_list )
		for x in meta_urls:
			name_suffix = re.findall( "[^-]*(-.*\\.png)", x[0] )
			name_suffix = name_suffix[0]
			fname = x[1]+name_suffix
			meta_url_dict[ fname ] = x[0]
	
	for image_name in meta_url_dict:
		print image_name,
		meta_url = meta_url_dict[ image_name ]
		try:
			image_file = open( image_name, "r" )
			image_file.close()
			print "[skip]"
			continue # do not load images again
		except:
			pass
		meta = urllib2.urlopen( urllib2.Request(base_url+meta_url, headers=headers) ).read()
		#                          <a href="http://upload.wikimedia.org/wikipedia/commons/6/6a/%E5%AD%A6-bw.png"><img alt="File:学-bw.png" 
		image_url = re.findall( """<a href="(http://upload[^"]*\\.png)"><img alt="File:%(image_name)s" """ % locals(), meta )
		try:
			image_url = image_url[0]
			image = urllib2.urlopen( urllib2.Request(image_url, headers=headers) ).read()
			image_file = open( image_name, "w" )
			image_file.write( image )
			image_file.close()
			print "[OK]"
		except IndexError, e:
			print "[FAIL: %s]" % str(e)

