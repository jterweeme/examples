TIME =
OPTIMIZE = -ftree-ter
OPTIMIZE = -O
WARNINGS = -Wall -Wno-parentheses -Wno-unused-function
DEBUG = -g

all: clean
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o compress compress.cpp
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o compress2 compress2.cpp
	gcc $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o extractc extractc.c
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o extractpp extractpp.cpp -std=c++20
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o lzw lzw.cpp
	gcc $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o zcatc zcatc.c
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o zcatpp zcatpp.cpp -std=c++20

clean:
	rm -vf compress compress2 extractc extractpp lzw zcatc zcatpp

test:
	$(TIME) ./zcatpp < accept.2.Z | md5sum -c accept.2.md5
	$(TIME) ./extract.py accept.2.Z | ./lzw | md5sum -c accept.2.md5
	$(TIME) ./zcat_min.py hocus.dat.Z | md5sum -c hocus.dat.md5
	$(TIME) ./zcat.py hocus.dat.Z | md5sum -c hocus.dat.md5
	$(TIME) ./extract.py hocus.dat.Z | ./lzw | md5sum -c hocus.dat.md5
	$(TIME) ./extractc hocus.dat.Z | ./lzw | md5sum -c hocus.dat.md5
	$(TIME) ./zcatc httpd-2.0.35.tar.Z | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) ./zcat.py httpd-2.0.35.tar.Z | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) ./zcat_min.py httpd-2.0.35.tar.Z | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) ./extract.py keen1.tar.Z | ./lzw | md5sum -c keen1.tar.md5
	$(TIME) ./zcatpp < logo.c.Z | md5sum -c logo.c.md5
	$(TIME) ./zcat_min.py logo.c.Z | md5sum -c logo.c.md5
	$(TIME) ./extractpp logo.c.Z | ./lzw | md5sum -c logo.c.md5
	$(TIME) ./zcatpp < mjkimage.c.Z | md5sum -c mjkimage.c.md5
	$(TIME) ./zcat_min.py mjkimage.c.Z | md5sum -c mjkimage.c.md5
	$(TIME) ./extract.py mjkimage.c.Z | ./lzw | md5sum -c mjkimage.c.md5
	$(TIME) ./zcat.py mo.m68.tar.Z | md5sum -c mo.m68.tar.md5
	$(TIME) ./zcat.py mo.ns32.tar.Z | md5sum -c mo.ns32.tar.md5
	$(TIME) ./zcat.py mo.sparc.tar.Z | md5sum -c mo.sparc.tar.md5
	$(TIME) ./zcatpp < mo.vax.tar.Z | md5sum -c mo.vax.tar.md5
	$(TIME) ./zcatpp < relnotes.ps.Z | md5sum -c relnotes.ps.md5
	$(TIME) ./zcat_min.py relnotes.ps.Z | md5sum -c relnotes.ps.md5
	$(TIME) ./zcat_min.py tty.1.Z | md5sum -c tty.1.md5
	$(TIME) ./zcatpp wk98.iso.Z16 | md5sum -c wk98.iso.md5
	$(TIME) ./extractpp wingames.iso.Z | ./lzw | md5sum -c wingames.iso.md5
	$(TIME) ./zcat_min.py wingames.iso.Z | md5sum -c wingames.iso.md5
	$(TIME) ./zcatpp enwik9.Z | md5sum -c enwik9.md5
	$(TIME) ./zcatpp hocus.dat.Z | ./compress2 | md5sum -c hocus.dat.Z.md5
	#$(TIME) ./zcatpp httpd-2.0.35.tar.Z | ./compress2 | md5sum -c httpd-2.0.35.tar.Z.md5
	$(TIME) ./zcatpp keen1.tar.Z | ./compress2 | md5sum -c keen1.tar.Z.md5
	$(TIME) ./zcatpp mjkimage.c.Z | ./compress2 | md5sum -c mjkimage.c.Z.md5
	$(TIME) ./zcatpp relnotes.ps.Z | ./compress2 | md5sum -c relnotes.ps.Z.md5
	#$(TIME) ./zcatpp wingames.iso.Z | ./compress2 | md5sum -c wingames.iso.Z.md5


