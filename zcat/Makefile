TIME = 
ZCATPP = zcatpp
ZCATC = zcatc
ZCATUX = zcatux

all:
	rm -vf $(ZCATPP) $(ZCATUX) $(ZCATC)
	g++ -g -O2 -Wall -Wno-parentheses -o $(ZCATPP) zcatpp.cpp
	g++ -g -O2 -Wall -Wno-parentheses -o $(ZCATUX) zcatux.cpp
	gcc -g -O2 -Wall -Wno-parentheses -o $(ZCATC) zcatc.c

test:
	$(TIME) zcat relnotes.ps.Z | md5sum
	$(TIME) ./$(ZCATPP) relnotes.ps.Z | md5sum
	$(TIME) ./$(ZCATUX) relnotes.ps.Z | md5sum
	$(TIME) ./$(ZCATC) relnotes.ps.Z | md5sum
	$(TIME) ./zcat.py relnotes.ps.Z | md5sum
	$(TIME) zcat keen1.tar.Z | md5sum
	$(TIME) ./$(ZCATPP) keen1.tar.Z | md5sum
	$(TIME) ./$(ZCATUX) keen1.tar.Z | md5sum
	$(TIME) ./$(ZCATC) keen1.tar.Z | md5sum
	$(TIME) ./zcat.py keen1.tar.Z | md5sum
	$(TIME) zcat DA40_cockpit.xml.Z | md5sum
	$(TIME) ./$(ZCATPP) DA40_cockpit.xml.Z | md5sum
	$(TIME) ./$(ZCATC) DA40_cockpit.xml.Z | md5sum
	$(TIME) ./zcat.py DA40_cockpit.xml.Z | md5sum
	$(TIME) zcat aa.txt.Z | md5sum
	$(TIME) ./$(ZCATPP) aa.txt.Z | md5sum
	$(TIME) ./$(ZCATC) aa.txt.Z | md5sum
	$(TIME) ./zcat.py aa.txt.Z | md5sum
	$(TIME) zcat tty.1.Z | md5sum
	$(TIME) ./$(ZCATPP) tty.1.Z | md5sum
	$(TIME) ./$(ZCATC) tty.1.Z | md5sum
	$(TIME) ./zcat.py tty.1.Z | md5sum
	$(TIME) zcat wk98.iso.Z | md5sum
	$(TIME) zcat side1.pnm.Z | md5sum
	$(TIME) ./$(ZCATPP) side1.pnm.Z | md5sum
	$(TIME) ./$(ZCATC) side1.pnm.Z | md5sum
	$(TIME) ./zcat.py side1.pnm.Z | md5sum
	$(TIME) zcat wingames.iso.Z | md5sum
	$(TIME) ./$(ZCATPP) wingames.iso.Z | md5sum
	$(TIME) ./zcat.py wingames.iso.Z | md5sum
	$(TIME) zcat enwik9.Z | md5sum


