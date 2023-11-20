all:
	g++ -O2 -Wall -Wno-parentheses -o zcat zcat.cpp
	gcc -O2 -Wall -Wno-parentheses -o zcatc zcat.c

test:
	./zcat < relnotes.ps.Z | md5sum
	zcat < relnotes.ps.Z | md5sum
	./zcat < keen1.tar.Z | md5sum
	zcat < keen1.tar.Z | md5sum
	./zcat < aa.txt.Z | md5sum
	zcat < aa.txt.Z | md5sum
	./zcat < tty.1.Z | md5sum
	zcat < tty.1.Z | md5sum

