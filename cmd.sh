#! /bin/bash
gcc -o joueur joueur.c `pkg-config --libs --cflags gtk+-2.0`
gcc -o mainserver mainserver.c -lpthread
