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
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o zcatcoro zcatcoro.cpp -std=c++20
	g++ $(DEBUG) $(OPTIMIZE) $(WARNINGS) -o zcatpp zcatpp.cpp -std=c++20

clean:
	rm -vf compress compress2 extractc extractpp lzw zcatc zcatcoro zcatpp

test:
	$(TIME) ./zcatpp < accept.2.Z | md5sum -c accept.2.sum
	$(TIME) ./zcatc hocus.dat.Z | md5sum -c hocus.dat.sum
	$(TIME) ./zcat.py keen1.tar.Z | md5sum -c keen1.tar.sum
	$(TIME) ./zcatpp < logo.c.Z | md5sum -c logo.c.sum
	$(TIME) ./zcat.py logo.c.Z | md5sum -c logo.c.sum
	$(TIME) ./extractpp logo.c.Z | ./lzw | md5sum -c logo.c.sum
	$(TIME) ./zcatpp < mjkimage.c.Z | md5sum -c mjkimage.c.sum
	$(TIME) ./zcat.py mjkimage.c.Z | md5sum -c mjkimage.c.sum
	$(TIME) ./extract.py mjkimage.c.Z | ./lzw.py | md5sum -c mjkimage.c.sum
	$(TIME) ./zcat.py mo.m68.tar.Z | md5sum -c mo.m68.tar.sum
	$(TIME) ./zcat.py mo.ns32.tar.Z | md5sum -c mo.ns32.tar.sum
	$(TIME) ./zcat.py mo.sparc.tar.Z | md5sum -c mo.sparc.tar.sum
	$(TIME) ./zcatpp < mo.vax.tar.Z | md5sum -c mo.vax.tar.sum
	$(TIME) ./zcatpp < relnotes.ps.Z | md5sum -c relnotes.ps.sum
	$(TIME) ./zcat.py relnotes.ps.Z | md5sum -c relnotes.ps.sum
	$(TIME) ./zcat.py tty.1.Z | md5sum -c tty.1.sum
	$(TIME) ./zcatpp wk98.iso.Z16 | md5sum -c wk98.iso.sum
	$(TIME) ./zcatcoro wk98.iso.Z16 | md5sum -c wk98.iso.sum
	$(TIME) ./extractpp wingames.iso.Z | ./lzw | md5sum -c wingames.iso.sum
	$(TIME) ./zcatpp enwik9.Z | md5sum -c enwik9.sum
	$(TIME) zcat hocus.dat.Z | ./compress2 | md5sum -c hocus.dat.Z.sum
	$(TIME) zcat keen1.tar.Z | ./compress2 | md5sum -c keen1.tar.Z.sum


