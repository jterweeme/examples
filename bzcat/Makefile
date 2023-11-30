all:
	g++ -fsanitize=address -Wall -Wno-parentheses -O2 -o bzcat bzcat.cpp
	g++ -fsanitize=address -Wall -Wno-parentheses -O2 -o bzcat2 bzcat2.cpp
	gcc -fsanitize=address -Wall -Wno-parentheses -O2 -o bzcatc bzcat.c
	javac Bzcat.java

clean:
	rm -v *.class bzcat

test:
	bzcat relnotes.ps.bz2 | md5sum
	./bzcat relnotes.ps.bz2 | md5sum
	#./bzcat2 relnotes.ps.bz2 | md5sum
	./bzcatc relnotes.ps.bz2 | md5sum
	java Bzcat relnotes.ps.bz2 | md5sum
	bzcat wingames.iso.bz2 | md5sum
	./bzcat wingames.iso.bz2 | md5sum
	#./bzcat2 wingames.iso.bz2 | md5sum
	./bzcatc wingames.iso.bz2 | md5sum
	java Bzcat wingames.iso.bz2 | md5sum
	#bzcat wk98.iso.bz2 | md5sum
	#./bzcat wk98.iso.bz2 | md5sum
	#./bzcat2 wk98.iso.bz2 | md5sum
	#./bzcatc wk98.iso.bz2 | md5sum
	#java Bzcat wk98.iso.bz2 | md5sum

