CC=cl

all:
	$(CC) /EHsc wod.cpp

debug:
	$(CC) /EHsc /Zi wod.cpp