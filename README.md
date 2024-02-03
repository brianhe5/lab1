## UID: 505804874

## Pipe Up

Program to imitate the pipe command funneling the output of one process into the input of the next process and so on.

## Building

Run make
Run ./pipe and subsequence processes such as ls, cat, wc, sort,etc.

## Running

./pipe ls cat wc returns  "     7        7      63      ", matching the output of
ls | cat | wc

## Cleaning up

Run 'make clean' to clean up all binary files