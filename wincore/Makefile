WARNINGS = -Wall -Wno-parentheses

all:
	g++ $(WARNINGS) -O -o bzcat bzcat.cpp -std=c++20
	g++ $(WARNINGS) -O -o compress compress.cpp -std=c++20
	g++ $(WARNINGS) -O -o flac2wav flac2wav.cpp -std=c++20
	g++ $(WARNINGS) -O -o gzcat gzcat.cpp -std=c++20
	g++ $(WARNINGS) -O -o md5sum md5sum.cpp -std=c++20
	g++ $(WARNINGS) -O -o pack pack.cpp -std=c++20
	g++ $(WARNINGS) -O -o pcat pcat.cpp -std=c++20
	g++ $(WARNINGS) -O -o zcat zcat.cpp -std=c++20

clean:
	rm -vf bzcat compress flac2wav gzcat md5sum pack pcat zcat

