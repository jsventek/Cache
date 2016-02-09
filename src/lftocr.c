/*
 * reads file specified as an argument (or stdin if not specified),
 * removes occurrences of \r
 * converts occurrences of \n to \r
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
      if (c == '\r')
         continue;
      else if (c == '\n')
         fputc('\r', stdout);
      else
         fputc(c, stdout);
   return 0;
}
