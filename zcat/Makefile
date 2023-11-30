all:
	rm -v zcat zcatc
	g++ -O2 -fsanitize=address,undefined -Wall -Wno-parentheses -o zcat zcat.cpp
	gcc -O2 -fsanitize=address,undefined -Wall -Wno-parentheses -o zcatc zcat.c

test:
	zcat relnotes.ps.Z | md5sum
	./zcat relnotes.ps.Z | md5sum
	./zcatc relnotes.ps.Z | md5sum
	zcat keen1.tar.Z | md5sum
	./zcat keen1.tar.Z | md5sum
	./zcatc keen1.tar.Z | md5sum
	zcat DA40_cockpit.xml.Z | md5sum
	./zcat DA40_cockpit.xml.Z | md5sum
	./zcatc DA40_cockpit.xml.Z | md5sum
	zcat aa.txt.Z | md5sum
	./zcat aa.txt.Z | md5sum
	./zcatc aa.txt.Z | md5sum
	zcat tty.1.Z | md5sum
	./zcat tty.1.Z | md5sum
	./zcatc tty.1.Z | md5sum
	zcat wk98.iso.Z | md5sum
	./zcat wk98.iso.Z | md5sum
	./zcatc wk98.iso.Z | md5sum
	zcat side1.pnm.Z | md5sum
	./zcat side1.pnm.Z | md5sum
	./zcatc side1.pnm.Z | md5sum
	zcat wingames.iso.Z | md5sum
	./zcat wingames.iso.Z | md5sum
	./zcatc wingames.iso.Z | md5sum
	zcat enwik9.Z | md5sum
	./zcat enwik9.Z | md5sum
	./zcatc enwik9.Z | md5sum


