all:
	g++ -Wall -Wno-parentheses -O2 -o bzcat bzcat.cpp
	g++ -Wall -Wno-parentheses -O2 -o bzcat2 bzcat2.cpp
	gcc -Wall -Wno-parentheses -O2 -o bzcatc bzcat.c
	javac Bzcat.java

clean:
	rm -v *.class bzcat

test:
	bzcat wk98.iso.bz2 | md5sum
	./bzcat wk98.iso.bz2 | md5sum
	./bzcat2 wk98.iso.bz2 | md5sum
	./bzcatc wk98.iso.bz2 | md5sum
	java Bzcat wk98.iso.bz2 | md5sum

