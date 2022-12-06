#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  bool number_empty;
  bool number_all;
  bool show_endl;
  bool squeeze;
  bool show_tabs;
  bool vflag;
} CatInfo;

bool CatParseArg(CatInfo *info, char *argv) {
  ++argv;
  if (*argv == '-') {
    ++argv;
    if (strcmp(argv, "number-nonblank") == 0) {
      info->number_empty = true;
      info->number_all = true;
    } else if (strcmp(argv, "number") == 0) {
      info->number_all = true;
    } else if (strcmp(argv, "squeeze-blank") == 0) {
      info->squeeze = true;
    } else {
      printf("cat: illegal option -- %s\nusage: cat [-benst] [file ...]", argv);
      return false;
    }

    return true;
  } else {
    for (char *it = argv; *it; ++it) {
      switch (*it) {
        case 'b':
          info->number_empty = true;
          info->number_all = true;
          break;
        case 'e':
          info->show_endl = true;
          info->vflag = true;
          break;
        case 'n':
          info->number_all = true;
          break;
        case 's':
          info->squeeze = true;
          break;
        case 't':
          info->show_tabs = true;
          info->vflag = true;
          break;
        case 'E':
          info->show_endl = true;
          break;
        case 'T':
          info->show_tabs = true;
          break;
        default:
          printf("cat: illegal option -- %s\nusage: cat [-benst] [file ...]",
                 argv);
          return false;
      }
    }
  }
  return true;
}

void CatNoArgs(int fd, char *argv) {
  char buf[4096];
  int bytes_read;

  if (fd == -1) {
    printf("cat: %s: No such file or directory\n", argv);
    return;
  }

  bytes_read = read(fd, buf, 4096);
  while (bytes_read > 0) {
    printf("%.*s", bytes_read, buf);
    bytes_read = read(fd, buf, 4096);
  }
}

bool CatArgsPerform(CatInfo info, char *file) {
  int ch, gobble, line, prev;
  FILE *fp = fopen(file, "rb");
  if (fp == NULL) {
    printf("cat: %s: No such file or directory\n", file);
    return false;
  }
  line = gobble = 0;
  for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
    if (prev == '\n') {
      if (info.squeeze) {
        if (ch == '\n') {
          if (gobble) continue;
          gobble = 1;
        } else
          gobble = 0;
      }
      if (info.number_all && (!info.number_empty || ch != '\n')) {
        (void)fprintf(stdout, "%6d\t", ++line);
        if (ferror(stdout)) break;
      }
    }
    if (ch == '\n') {
      if (info.show_endl && putchar('$') == EOF) break;
    } else if (ch == '\t') {
      if (info.show_tabs) {
        if (putchar('^') == EOF || putchar('I') == EOF) break;
        continue;
      }
    } else if (info.vflag) {
      if (!isascii(ch) && !isprint(ch)) {
        if (putchar('M') == EOF || putchar('-') == EOF) break;
        ch = toascii(ch);
      }
      if (iscntrl(ch)) {
        if (putchar('^') == EOF || putchar(ch == 127 ? '?' : ch | 64) == EOF)
          break;
        continue;
      }
    }
    if (putchar(ch) == EOF) break;
  }
  fclose(fp);

  return true;
}

bool CatArgs(int argc, char *argv[]) {
  CatInfo info = {
      0, 0, 0, 0, 0, 0,
  };
  int count = 1;
  for (int i = 1; i != argc; ++i) {
    if (*argv[i] == '-') {
      count++;
      if (!CatParseArg(&info, argv[i])) {
        return false;
      }
    }
    if (count == argc) {
      CatNoArgs(STDIN_FILENO, argv[0]);
    }
  }

  if (!(info.number_all + info.number_empty + info.show_endl + info.show_tabs +
        info.squeeze)) {
    for (int i = 1; i != argc; ++i) {
      if (*argv[i] != '-') {
        CatNoArgs(open(argv[i], O_RDONLY), argv[i]);
      }
    }

  } else {
    for (int i = 1; i != argc; ++i) {
      if (*argv[i] != '-') {
        CatArgsPerform(info, argv[i]);
      }
    }
  }

  return true;
}

int main(int argc, char *argv[]) {
  setlocale(LC_CTYPE, "");
  if (argc == 1) {
    CatNoArgs(STDIN_FILENO, argv[0]);
  } else {
    CatArgs(argc, argv);
  }

  return 0;
}
