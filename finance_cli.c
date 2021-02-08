#include "finance.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

const char *help = "\
Usage: finance [--help] <command> [args]\n\
Work on the finance database.\n\
\n\
Commands:\n\
  add <date> <amount> <description>: adds a new entry in the database\n\
  csv <outputfile>: exports the database to a csv file\n\
  show [-l [<limit>]]: Prints the database to stdout\n\
  sql <query>: Executes an sql query on the database\
";

const char *usage_add = "\
Usage: finance add <date> <amount> <description>\n\
  date: date in YYYY-MM-DD format or now, today or yesterday\n\
  amount: float value\n\
  description: Descriptions about the transaction\
";

const char *usage_csv = "\
Usage: finance csv <outputfile>\n\
  outputfile: the output csv file\
";

const char *usage_show = "\
Usage: finance show [-l [<limit>]] [-s]\n\
  -l: limit the amount of rows printed to the last <limit> entries. Default=10\n\
      if <limit> isn't specified but -l is, all rows are printed.\n\
  -s: also calculate the sum/current balance\
";

const char *usage_sql = "\
Usage: finance sql <query>\n\
  <query> any valid sql query\n\
\n\
The structure of the database is:\n\
            finance\n\
-----------------------------------\n\
id | date | amount | description\n\
";

int valid_date_format(char** date);
int isNumber(char* number);

int main(int argc, char **argv) {

  #ifdef _DEBUG
  puts("Debug mode");
  #endif /* _DEBUG */

  // Prints the help and exit
  if (argc < 2) {
    printf("%s", help);
    return 0;
  }
  if (!strcmp(argv[1], "--help")) {
    if (argc < 3) {
      puts(help);
      return 0;
    }
    if (!strcmp(argv[2], "add")) {
      puts(usage_add);
      return 0;
    }
    if (!strcmp(argv[2], "csv")) {
      puts(usage_csv);
      return 0;
    }
    if (!strcmp(argv[2], "show")) {
      puts(usage_show);
      return 0;
    }
    if (!strcmp(argv[2], "sql")) {
      puts(usage_sql);
      return 0;
    }
    printf("Unkown command: %s\n", argv[2]);
    puts(help);
    return 0;
  }

  // finance add <date> <amount> <description>
  if (!strcmp(argv[1], "add")) {
    // Check if there are enough arguments
    if (argc < 5) {
      puts("Not enough arguments.");
      printf("%s", usage_add);
      return 0;
    }

    // Check if the date is in a valid format
    char *date = argv[2];
    if (!valid_date_format(&date)) {
      puts("Please enter a valid date format.");
      printf("%s\n", usage_add);
      return 0;
    }

    // Executes the transaction
    if (open_db()) {
      close_db();
      return 1;
    }
    if (add(date, strtof(argv[3], NULL), argv[4])) {
      close_db();
      return 1;
    }
    close_db();
    return 0;
  }

  // finance csv <outputfile>
  if (!strcmp(argv[1], "csv")) {
    if (argc < 3) {
      puts("No outputfile specified.");
      printf("%s", usage_csv);
      return 0;
    }
    if (open_db()) {
      close_db();
      return 1;
    }
    if (export_to_csv(argv[2])) {
      close_db();
      return 1;
    }
    close_db();
    return 0;
  }

  // finance show [-l [<limit>]] [-s]
  if( !strcmp(argv[1], "show") ) {
    int limit = 10;
    int calculateSum = 0;
    size_t optind;
    for (optind = 2; optind < argc && argv[optind][0] == '-'; optind++) {
      switch (argv[optind][1]) {
        case 'l':
          // No limit given
          if (optind+1 == argc || argv[optind+1][0] == '-') {
            limit = 0;
            break;
          }
          if (!isNumber(argv[optind+1])) {
            printf("%s isn't a number.\n", argv[optind+1]);
            puts(usage_show);
            return 0;
          }
          limit = atoi(argv[optind + 1]);
          optind++;
          break;

        case 's': calculateSum = 1; break;

        default:
          printf("Unknown argument %s.\n", argv[optind]);
          puts(usage_show);
          return 0;
      }
    }

    if (open_db()) {
      close_db();
      return 1;
    }
    if (print_db(limit, calculateSum)) {
      close_db();
      return 1;
    }
    close_db();
    return 0;
  }

  // finance sql <query>
  if (!strcmp(argv[1], "sql")) {
    if (argc < 3) {
      puts("No query given.");
      puts(usage_sql);
      return 0;
    }
    if (open_db()) {
      close_db();
      return 1;
    }
    if (execute_query(argv[2], NULL, 0)) {
      close_db();
      return 1;
    }
    close_db();
    return 0;
  }

  // Invalid argument
  puts("Invalid argument given.");
  printf("%s", help);
  return 0;
}

int isNumber(char* number) {
  int i = 0;
  int isDigit = 1;
  while (i < strlen(number) && isDigit) {
    isDigit = isdigit(number[i]);
    i++;
  }
  return isDigit;
}

int valid_date_format (char** date) {
  if (!strcmp(*date, "yesterday")) {
    *date = "now\", \"-1 day";
    return 1;
  }
  if (!strcmp(*date, "today")) {
    *date = "now";
  }

  pcre2_code *re;
  PCRE2_SPTR pattern;
  PCRE2_SPTR subject;

  int errornumber;
  int rc;

  PCRE2_SIZE erroroffset;
  PCRE2_SIZE subject_length;

  pcre2_match_data *match_data;

  pattern = (PCRE2_SPTR)"^\\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[1 2]\\d|3[0-1])|now$";
  subject = (PCRE2_SPTR) *date;
  subject_length =  (PCRE2_SIZE)strlen((char*)subject);

  re = pcre2_compile(
    pattern,
    PCRE2_ZERO_TERMINATED,
    0,
    &errornumber,
    &erroroffset,
    NULL
  );

  if (re == NULL) {
    PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
    printf("PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset, buffer);
    return -1;
  }

  match_data = pcre2_match_data_create_from_pattern(re, NULL);

  rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  if (rc < 0) {
    if (rc == PCRE2_ERROR_NOMATCH) {
      return 0;
    }
    printf("Matching error %d\n", rc);
    return -1;
  }

  return (subject_length == 10 || subject_length == 3);

}