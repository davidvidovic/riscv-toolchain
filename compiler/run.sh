#! /usr/bin/bash

yacc -d riskyc.y
lex riskyc.l
gcc y.tab.c lex.yy.c -ll -lm
./a.out
