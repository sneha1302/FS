all: ramdisk

ramdisk:
	g++ -Wall ramdisk.cpp `pkg-config fuse --cflags --libs` -o ramdisk

postmark:
	gcc postmark.c -o postmark
