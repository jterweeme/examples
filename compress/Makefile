all:
	rm -vf compress compress2
	g++ -O2 -fsanitize=address -Wall -Wno-parentheses -o compress compress.cpp
	g++ -O2 -fsanitize=address -Wall -Wno-parentheses -o compress2 compress2.cpp

test:
	./compress2 < hocus.dat | md5sum

