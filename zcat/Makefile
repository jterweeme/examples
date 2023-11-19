all:
	g++ -O2 -Wall -Wno-parentheses -o zcat zcat.cpp

test:
	./zcat < relnotes.ps.Z | md5sum
	zcat < relnotes.ps.Z | md5sum
	./zcat < keen1.tar.Z | md5sum
	zcat < keen1.tar.Z | md5sum
	./zcat < aa.txt.Z | md5sum
	zcat < aa.txt.Z | md5sum

