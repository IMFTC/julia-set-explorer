julia-explorer: main.c
	gcc `pkg-config --cflags gtk+-3.0` -o \
	julia-explorer main.c `pkg-config --libs gtk+-3.0` -g
