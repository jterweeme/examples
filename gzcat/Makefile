all:
	g++ -fsanitize=address -O2 -Wall -Wno-parentheses -o gzcat gzcat.cpp
	javac Gzcat.java

test:
	zcat batterycheck.exe.gz | md5sum
	./gzcat batterycheck.exe.gz | md5sum
	java Gzcat batterycheck.exe.gz | md5sum
	zcat DA40_cockpit.xml.gz | md5sum
	./gzcat DA40_cockpit.xml.gz | md5sum
	java Gzcat DA40_cockpit.xml.gz | md5sum
	#zcat nfshs.bin.gz | md5sum
	#./gzcat nfshs.bin.gz | md5sum
	#java Gzcat nfshs.bin.gz | md5sum
	zcat Setup.exe.gz | md5sum
	./gzcat Setup.exe.gz | md5sum
	java Gzcat Setup.exe.gz | md5sum
	#zcat wk98.iso.gz | md5sum
	#./gzcat wk98.iso.gz | md5sum
	#java Gzcat wk98.iso.gz | md5sum

