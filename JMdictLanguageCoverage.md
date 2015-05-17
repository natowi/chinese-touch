
```
[thomas@1810t JMdict-20100709]$ for l in ain alg ara bnt bur chi dan dut eng epo fre ger \
gre heb hin hun ind ita khm kor lat may mon nor per pol por rus san spa swe tah tha tib \
tur vie; do echo $l $(cat JMdict.xml |grep "lang=\"$l\"" |wc -l); done
ain 11
alg 1
ara 34
bnt 1
bur 1
chi 40
dan 1
dut 82
eng 13
epo 1
fre 31805
ger 185445
gre 33
heb 3
hin 9
hun 3
ind 3
ita 210
khm 1
kor 29
lat 185
may 4
mon 2
nor 3
per 1
pol 1
por 67
rus 9526
san 24
spa 53
swe 2
tah 1
tha 5
tib 1
tur 2
vie 1
[thomas@1810t JMdict-20100709]$ cat JMdict.xml |grep '<gloss' |wc -l
469653
[thomas@1810t JMdict-20100709]$ cat JMdict.xml |grep '<gloss' |grep -v lang |wc -l
243380
```
The final number (243380) should show the number of English glosses without an explicit language specification (English is default).