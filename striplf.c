/*
 * reads file specified as an argument (or stdin if not specified),
 * converting newline characters to blanks,
 * and collapsing multiple sequences of white space to a single blank
 */
#include <stdio.h>

int main(int argc, char *argv[]) {
   int c;
   FILE *fd;

   if (argc > 2) {
      fprintf(stderr, "usage: %s [file]\n", argv[0]);
      return -1;
   }
   if (argc == 1)
      fd = stdin;
   else if (! (fd = fopen(argv[1], "r"))) {
      fprintf(stderr, "%s: unable to open file %s\n", argv[0], argv[1]);
      return -2;
   }
   while ((c = fgetc(fd)) != EOF)
      if (c == '\n')
         fputc(' ', stdout);
      else
         fputc(c, stdout);
   return 0;
}
