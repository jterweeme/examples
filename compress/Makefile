TIME =
OPTIMIZE = -ftree-ter
OPTIMIZE = -O
WARNINGS = -Wall -Wno-parentheses -Wno-unused-function
DEBUG = -g

all: clean
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o codify codify.cpp
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o codify2 codify2.cpp
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o compress compress.cpp
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o compress2 compress2.cpp
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o compress3 compress3.cpp -std=c++20
	gcc $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o extractc extractc.c
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o lzw lzw.cpp -std=c++20
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o press press.cpp -std=c++20
	gcc $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o zcatc zcatc.c
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o zcatpp zcatpp.cpp -std=c++20

clean:
	rm -vf codify codify2 compress compress2 compress3 extractc lzw press zcatc zcatpp

test:
	$(TIME) ./extractc accept.2.Z | ./press | md5sum -c accept.2.Z.md5
	$(TIME) ./extractc accept.2.Z | ./press.py | md5sum -c accept.2.Z.md5
	$(TIME) ./extractc hocus.dat.Z | ./press.py | md5sum -c hocus.dat.Z.md5
	#$(TIME) ./extractc wingames.iso.Z | ./press.py | md5sum -c wingames.iso.Z.md5
	#$(TIME) ./extractc wk98.iso.Z | ./press | md5sum -c wk98.iso.Z.md5
	#$(TIME) ./extractc wk98.iso.Z | ./press.py | md5sum -c wk98.iso.Z.md5
	$(TIME) ./zcatc < accept.2.Z | md5sum -c accept.2.md5
	$(TIME) ./zcatpp < accept.2.Z | md5sum -c accept.2.md5
	$(TIME) ./zcat.py accept.2.Z | md5sum -c accept.2.md5
	$(TIME) ./zcat_min.py hocus.dat.Z | md5sum -c hocus.dat.md5
	$(TIME) ./zcat.py hocus.dat.Z | md5sum -c hocus.dat.md5
	$(TIME) ./extractc hocus.dat.Z | ./lzw | md5sum -c hocus.dat.md5
	$(TIME) ./zcatc httpd-2.0.35.tar.Z | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) ./zcat.py httpd-2.0.35.tar.Z | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) ./zcat_min.py httpd-2.0.35.tar.Z | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) ./zcatpp < logo.c.Z | md5sum -c logo.c.md5
	$(TIME) ./zcat_min.py logo.c.Z | md5sum -c logo.c.md5
	$(TIME) ./extractc logo.c.Z | ./lzw | md5sum -c logo.c.md5
	$(TIME) ./zcatpp < mjkimage.c.Z | md5sum -c mjkimage.c.md5
	$(TIME) ./zcat_min.py mjkimage.c.Z | md5sum -c mjkimage.c.md5
	$(TIME) ./zcat.py mo.m68.tar.Z | md5sum -c mo.m68.tar.md5
	$(TIME) ./zcat.py mo.ns32.tar.Z | md5sum -c mo.ns32.tar.md5
	$(TIME) ./zcat.py mo.sparc.tar.Z | md5sum -c mo.sparc.tar.md5
	$(TIME) ./zcatpp < mo.vax.tar.Z | md5sum -c mo.vax.tar.md5
	$(TIME) ./zcatpp < relnotes.ps.Z | md5sum -c relnotes.ps.md5
	$(TIME) ./zcat_min.py relnotes.ps.Z | md5sum -c relnotes.ps.md5
	$(TIME) ./zcat_min.py tty.1.Z | md5sum -c tty.1.md5
	#$(TIME) ./zcatc wk98.iso.Z | md5sum -c wk98.iso.md5
	#$(TIME) ./zcatpp wk98.iso.Z | md5sum -c wk98.iso.md5
	$(TIME) ./extractc wingames.iso.Z | ./lzw | md5sum -c wingames.iso.md5
	#$(TIME) ./zcat_min.py wingames.iso.Z | md5sum -c wingames.iso.md5
	#$(TIME) ./zcatpp enwik9.Z | md5sum -c enwik9.md5
	$(TIME) zcat accept.2.Z | ./compress3 | md5sum -c accept.2.Z.md5
	$(TIME) zcat hocus.dat.Z | ./codify2 | ./press | md5sum -c hocus.dat.Z.md5
	$(TIME) zcat hocus.dat.Z | ./compress3 | zcat | md5sum -c hocus.dat.md5
	$(TIME) zcat httpd-2.0.35.tar.Z | ./codify | ./press | zcat | md5sum -c httpd-2.0.35.tar.md5
	$(TIME) zcat keen1.tar.Z | ./codify2 | ./press | md5sum -c keen1.tar.Z.md5
	$(TIME) zcat keen1.tar.Z | ./compress3 | zcat | md5sum -c keen1.tar.md5
	$(TIME) zcat logo.c.Z | ./compress3 | md5sum -c logo.c.Z.md5
	$(TIME) zcat mjkimage.c.Z | ./codify2 | ./press | md5sum -c mjkimage.c.Z.md5
	$(TIME) zcat mo.m68.tar.Z | ./codify2 | ./press | zcat | md5sum -c mo.m68.tar.md5
	$(TIME) zcat tty.1.Z | ./compress3 | md5sum -c tty.1.Z.md5
	$(TIME) zcat wingames.iso.Z | ./codify | ./press | md5sum -c wingames.iso.Z.md5
	#$(TIME) zcat wk98.iso.Z | ./codify | ./press | md5sum -c wk98.iso.Z.md5


