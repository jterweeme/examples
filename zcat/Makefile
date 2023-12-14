TIME = 

all:
	rm -vf zcat zcat2 zcatc
	g++ -g -O2 -Wall -Wno-parentheses -o zcat zcat.cpp
	#g++ -g -O2 -Wall -Wno-parentheses -o zcat2 zcat2.cpp
	gcc -g -O2 -fsanitize=address,undefined -Wall -Wno-parentheses -o zcatc zcat.c

test:
	$(TIME) zcat relnotes.ps.Z | md5sum
	$(TIME) ./zcat relnotes.ps.Z | md5sum
	$(TIME) ./zcat.py relnotes.ps.Z | md5sum
	#./zcatc relnotes.ps.Z | md5sum
	$(TIME) zcat keen1.tar.Z | md5sum
	$(TIME) ./zcat keen1.tar.Z | md5sum
	$(TIME) ./zcat.py keen1.tar.Z | md5sum
	#./zcatc keen1.tar.Z | md5sum
	$(TIME) zcat DA40_cockpit.xml.Z | md5sum
	$(TIME) ./zcat DA40_cockpit.xml.Z | md5sum
	$(TIME) ./zcat.py DA40_cockpit.xml.Z | md5sum
	#./zcatc DA40_cockpit.xml.Z | md5sum
	$(TIME) zcat aa.txt.Z | md5sum
	$(TIME) ./zcat aa.txt.Z | md5sum
	$(TIME) ./zcat.py aa.txt.Z | md5sum
	#./zcatc aa.txt.Z | md5sum
	$(TIME) zcat tty.1.Z | md5sum
	$(TIME) ./zcat tty.1.Z | md5sum
	$(TIME) ./zcat.py tty.1.Z | md5sum
	#./zcatc tty.1.Z | md5sum
	$(TIME) zcat wk98.iso.Z | md5sum
	#./zcat wk98.iso.Z | md5sum
	#./zcatc wk98.iso.Z | md5sum
	$(TIME) zcat side1.pnm.Z | md5sum
	$(TIME) ./zcat side1.pnm.Z | md5sum
	$(TIME) ./zcat.py side1.pnm.Z | md5sum
	#./zcatc side1.pnm.Z | md5sum
	$(TIME) zcat wingames.iso.Z | md5sum
	$(TIME) ./zcat wingames.iso.Z | md5sum
	$(TIME) ./zcat.py wingames.iso.Z | md5sum
	#./zcatc wingames.iso.Z | md5sum
	$(TIME) zcat enwik9.Z | md5sum
	#./zcat enwik9.Z | md5sum
	#./zcatc enwik9.Z | md5sum


