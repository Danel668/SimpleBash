#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  bool iflag;
  bool vflag;
  bool cflag;
  bool lflag;
  bool nflag;
} GrepInfo;

bool flag_handing(int argc, char *argv[], GrepInfo *info, char *template) {
  int rez = 0;
  const char *short_options = "ivclne:";
  bool isE = false;

  while ((rez = getopt_long(argc, argv, short_options, NULL, NULL)) != -1) {
    switch (rez) {
      case 'e':
        if (isE) strcat(template, "|");
        if (*optarg == '\0') {
          strcat(template, ".");
        } else {
          strcat(template, optarg);
        }
        isE = true;
        break;
      case 'i':
        info->iflag = 1;
        break;
      case 'v':
        info->vflag = 1;
        break;
      case 'c':
        info->cflag = 1;
        break;
      case 'l':
        info->lflag = 1;
        break;
      case 'n':
        info->nflag = 1;
        break;
      case '?':
        return false;
        break;
    }
  }
  if (!isE) {
    strcat(template, argv[optind]);
    optind++;
  }
  return true;
}

void f_printf(int argc, char *argv[], GrepInfo info, char *template) {
  regex_t regex;  // область хранения скомпил рег выражения
  int cflags = REG_EXTENDED;  // тип компиляции
  if (info.iflag) {
    cflags = REG_ICASE;
  }
  regcomp(&regex, template, cflags);

  size_t nmatch = 1;
  regmatch_t pmatch[1];
  int N_str = 1;         // номер строки
  int N_str_find = 0;    // номер строки совпадения
  int N_str_nofind = 0;  // номер строки не совпадения
  int success;
  int N_optind = argc - optind;

  while (optind < argc) {
    char str_file[10000] = {0};  //  целевая текстовая строка
    FILE *file = NULL;
    file = fopen(argv[optind], "r");

    if (file == NULL) {
      fprintf(stderr, "grep: %s: No such file or derectory\n", argv[optind]);
    } else {
      while (fgets(str_file, sizeof str_file, file) != NULL) {
        success = regexec(&regex, str_file, nmatch, pmatch, 0);
        // добавим в конце файла перенос строки
        if (strchr(str_file, '\n') == 0) {
          strcat(str_file, "\n");
        }

        if ((N_optind > 1 && success == 0 && !info.lflag && !info.vflag &&
             !info.cflag)) {
          printf("%s:", argv[optind]);
        }

        if (N_optind > 1 && success != 0 && info.vflag && !info.lflag &&
            !info.cflag) {
          printf("%s:", argv[optind]);
        }

        // n flag does not work with cflag and lflag
        if (info.nflag && !info.cflag && !info.lflag &&
            ((success == 0 && !info.vflag) || (success != 0 && info.vflag))) {
          printf("%d:", N_str);
        }

        // печать без флагов
        if (success == 0 && (info.vflag + info.cflag + info.lflag) == 0) {
          printf("%s", str_file);
        }

        // печатать с -v
        if (success != 0 && (info.cflag + info.lflag) == 0 && info.vflag) {
          printf("%s", str_file);
        }

        if (success == 0) {
          N_str_find++;
        }
        if (success != 0) {
          N_str_nofind++;
        }
        N_str++;
      }

      // -c
      if (info.cflag && !info.vflag && !info.lflag) {
        if (N_optind > 1) {
          printf("%s:", argv[optind]);
        }
        printf("%d\n", N_str_find);
      }
      // -c -l
      if (info.cflag && info.lflag && !info.vflag) {
        if (N_optind > 1) {
          printf("%s:", argv[optind]);
        }
        if (N_str_find > 0) {
          printf("1\n");
          printf("%s\n", argv[optind]);
        } else {
          printf("0\n");
        }
      }
      // c and !v
      if (info.cflag && info.vflag && !info.lflag) {
        if (N_optind > 1) {
          printf("%s:", argv[optind]);
        }
        printf("%d\n", N_str_nofind);
      }

      // -c -l -v
      if (info.cflag && info.vflag && info.lflag) {
        if (N_optind > 1) {
          printf("%s:", argv[optind]);
        }
        printf("1\n");
        printf("%s\n", argv[optind]);
      }

      // -l and -v
      if (info.lflag && info.vflag && N_str_nofind > 0 && !info.cflag) {
        printf("%s\n", argv[optind]);
      }

      if (info.lflag && !info.vflag && N_str_find > 0 && !info.cflag) {
        printf("%s\n", argv[optind]);
      }

      fclose(file);
      N_str = 1;
      N_str_find = 0;
      N_str_nofind = 0;
    }
    optind++;
  }
  regfree(&regex);
}

int main(int argc, char *argv[]) {
  setlocale(LC_CTYPE, "");
  if (argc > 2) {
    GrepInfo info = {0, 0, 0, 0, 0};
    char template[1024];
    template[0] = '\0';
    if (flag_handing(argc, argv, &info, template)) {
      f_printf(argc, argv, info, template);
    } else {
      printf("usage: grep [-ivclne] [file ...]\n");
    }
  } else {
    printf("usage: grep [-ivclne] [pattern] [file ...]\n");
  }
  return 0;
}
