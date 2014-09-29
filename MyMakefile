all: fs_stat

fs_stat: main.o Disk.o ext.o FS.o ntfs.o
	g++ -w -std=c++0x main.o Disk.o ext.o FS.o ntfs.o -o fs_stat

main.o: main.cpp FS.h fs_structs.h
	g++ -w -std=c++0x -c main.cpp -o main.o

Disk.o: Disk.cpp FS.h fs_structs.h
	g++ -w -std=c++0x -c Disk.cpp -o Disk.o

ext.o: ext.cpp FS.h fs_structs.h
	g++ -w -std=c++0x -c ext.cpp -o ext.o

FS.o: FS.cpp FS.h fs_structs.h
	g++ -w -std=c++0x -c FS.cpp -o FS.o
	
ntfs.o: ntfs.cpp FS.h fs_structs.h
	g++ -w -std=c++0x -c ntfs.cpp -o ntfs.o

clean:
	rm -rf main.o Disk.o ext.o FS.o ntfs.o fs_stat
