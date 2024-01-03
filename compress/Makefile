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
	./compress2 < hocus.dat | md5sum
	md5sum hocus.dat.Z
	./compress2 < keen1.tar | md5sum
	md5sum keen1.tar.Z
	$(TIME) zcat < logo.c.Z | md5sum
	$(TIME) ./$(ZCATPP) < logo.c.Z | md5sum
	$(TIME) ./zcat.py logo.c.Z | md5sum
	$(TIME) ./extract_coro logo.c.Z | ./lzw | md5sum
	$(TIME) zcat < mjkimage.c.Z | md5sum
	$(TIME) ./$(ZCATPP) < mjkimage.c.Z | md5sum
	$(TIME) ./zcat.py mjkimage.c.Z | md5sum
	$(TIME) ./extract_codes.py mjkimage.c.Z | ./lzw.py | md5sum
	$(TIME) zcat < mo.m68.tar.Z | md5sum
	$(TIME) ./$(ZCATPP) < mo.m68.tar.Z | md5sum
	$(TIME) ./zcat.py mo.m68.tar.Z | md5sum
	$(TIME) zcat < mo.ns32.tar.Z | md5sum
	$(TIME) ./$(ZCATPP) < mo.ns32.tar.Z | md5sum
	$(TIME) ./zcat.py mo.ns32.tar.Z | md5sum
	$(TIME) zcat < mo.sparc.tar.Z | md5sum
	$(TIME) ./$(ZCATPP) < mo.sparc.tar.Z | md5sum
	$(TIME) ./zcat.py mo.sparc.tar.Z | md5sum
	$(TIME) zcat < mo.vax.tar.Z | md5sum
	$(TIME) ./$(ZCATPP) < mo.vax.tar.Z | md5sum
	$(TIME) ./zcat.py mo.vax.tar.Z | md5sum
	$(TIME) zcat < relnotes.ps.Z | md5sum
	$(TIME) ./$(ZCATPP) < relnotes.ps.Z | md5sum
	$(TIME) ./zcat.py relnotes.ps.Z | md5sum
	$(TIME) zcat keen1.tar.Z | md5sum
	$(TIME) ./$(ZCATPP) keen1.tar.Z | md5sum
	$(TIME) ./zcat.py keen1.tar.Z | md5sum
	$(TIME) zcat DA40_cockpit.xml.Z | md5sum
	$(TIME) ./$(ZCATPP) DA40_cockpit.xml.Z | md5sum
	$(TIME) ./zcat.py DA40_cockpit.xml.Z | md5sum
	$(TIME) zcat aa.txt.Z | md5sum
	$(TIME) ./$(ZCATPP) aa.txt.Z | md5sum
	$(TIME) ./zcat.py aa.txt.Z | md5sum
	$(TIME) zcat tty.1.Z | md5sum
	$(TIME) ./$(ZCATPP) tty.1.Z | md5sum
	$(TIME) ./zcat.py tty.1.Z | md5sum
	$(TIME) zcat wk98.iso.Z | md5sum
	$(TIME) ./$(ZCATPP) wk98.iso.Z | md5sum
	$(TIME) ./zcatcoro wk98.iso.Z | md5sum
	$(TIME) zcat side1.pnm.Z | md5sum
	$(TIME) ./$(ZCATPP) side1.pnm.Z | md5sum
	#$(TIME) ./zcat.py side1.pnm.Z | md5sum
	$(TIME) zcat wingames.iso.Z | md5sum
	$(TIME) ./$(ZCATPP) wingames.iso.Z | md5sum
	$(TIME) ./extract_coro wingames.iso.Z | ./lzw | md5sum
	#$(TIME) ./zcat.py wingames.iso.Z | md5sum
	$(TIME) zcat enwik9.Z | md5sum
	$(TIME) ./$(ZCATPP) enwik9.Z | md5sum


