#include "finance.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>

#include "sqlite/sqlite3.h"

#define OUTPUT_METHOD "table"

char *HOME;
char *FINANCE_DATABASE_LOCATION;
char *FINANCE_DATABASE;

sqlite3 *db;
char *zErrMsg;
int rc;

int open_db() {
  // Gets the HOME directory and get the database location at $HOME/.local/share/finance/finance.db
  if ((HOME = getenv("HOME")) == NULL) {
    printf("HOME env variable not set.\n");
    HOME = getpwuid(getuid())->pw_dir;
  }
  #ifdef _DEBUG
  FINANCE_DATABASE_LOCATION = "./";
  #else
  if( 0 > asprintf(&FINANCE_DATABASE_LOCATION, "%s/%s", HOME, ".local/share/finance/")) return 1;
  #endif /* _DEBUG */
  if( 0 > asprintf(&FINANCE_DATABASE, "%s/%s", FINANCE_DATABASE_LOCATION, "finance.db")) return 1;
  rc = mkdir(FINANCE_DATABASE_LOCATION, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if( rc ) {
    if (errno != EEXIST) {
      fprintf(stderr, "Error creating directory (%d)\n", errno);
      return 1;
    }
  }

  // Open the database and create one if necessary
  rc = sqlite3_open(FINANCE_DATABASE, &db);
  if( rc ) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return 1;
  }

  // If the database doesn't have the table yet (freshly created), create the table
  rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS finance(id INTEGER PRIMARY KEY ASC, date DATE, amount REAL, description TEXT);", NULL, 0, &zErrMsg);
  if ( rc!=SQLITE_OK ) {
    fprintf(stderr, "SQL error(%d): %s\n", rc, zErrMsg);
    sqlite3_free(zErrMsg);
    return 1;
  }

  return 0;
}

void close_db() {
  sqlite3_close(db);
  db = NULL;
}

int add(char* date, float amount, char* description) {
  
  // Formulate the INSERT query
  char* sql;
  asprintf(&sql, "INSERT INTO finance(date, amount, description) VALUES(DATE(\"%s\"), %f, \"%s\");", date, amount, description);
  
  // Execute the query
  rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
  if ( rc!=SQLITE_OK ) {
    fprintf(stderr, "SQL error(%d): %s\n", rc, zErrMsg);
    sqlite3_free(zErrMsg);
    return 1;
  }
  return 0;
}

#ifndef EXTERN_SQLITE
/* Callback for the export_to_csv function */
static int export_to_csv_callback(void* of_void,  int argc, char **argv, char **azColName) {
  // Passes the output filestream as a void*
  FILE *of = (FILE*)of_void;
  for (int i = 0; i < argc; i++) {
    fprintf(of, "%s", argv[i]);
    if (i != argc - 1) {
      fprintf(of, ",");
    }
  }
  fprintf(of, "\n");
  return 0;
}
#endif /* EXTERN_SQLITE */

// sqlite3 -header -csv FINANCE_DATABASE "SELECT * FROM finance;" > filename
int export_to_csv(char* filename) {
#ifdef EXTERN_SQLITE
  char* system_call;
  asprintf(&system_call, "sqlite3 -header -csv %s \"SELECT * FROM finance;\" > %s", FINANCE_DATABASE, filename);
  FILE* fp = popen(system_call, "r");
  if (fp == NULL) {
    perror("Error executing sqlite3 command");
    return 1;
  }
  return pclose(fp);
#else /* EXTERN_SQLITE */

  FILE *of = fopen(filename, "w");
  // Could maybe do with query, but the database will stay the same, so no need for extra work
  fprintf(of, "id,date,amount,description\n");
  rc = execute_query("SELECT * FROM finance;", export_to_csv_callback, of);
  fclose(of);
  return rc;

#endif /* EXTERN_SQLITE */
}

#ifndef EXTERN_SQLITE
static int print_db_callback(void* unused, int argc, char **argv, char **azColName) {
  printf("|%3d | %s | %+8.2f | %s\n", atoi(argv[0]), argv[1], atof(argv[2]), argv[3]);
  return 0;
}
#endif /* EXTERN_SQLITE */

static int calculate_sum_callback(void* unused, int argc, char **argv, char **azColName) {
  printf("Balance = €%8.2f\n", atof(argv[0]));
  return 0;
}

int print_db(int limit, int calculateSum) {
  char *sql;
#ifdef EXTERN_SQLITE
  if (limit) {
    asprintf(&sql, "SELECT id, date, printf(\\\"%%+8.2f\\\", amount) as amount, description FROM (SELECT * FROM finance ORDER BY id DESC LIMIT %d) ORDER BY id ASC;", limit);
  } else {
    asprintf(&sql, "SELECT id, date, printf(\\\"%%+8.2f\\\", amount) as amount, description FROM finance;");
  }
#else /* EXTERN_SQLITE */
  if (limit) {
    asprintf(&sql, "SELECT id, date, printf(\"%%+8.2f\", amount) as amount, description FROM (SELECT * FROM finance ORDER BY id DESC LIMIT %d) ORDER BY id ASC;", limit);
  } else {
    asprintf(&sql, "SELECT id, date, printf(\"%%+8.2f\", amount) as amount, description FROM finance;");
  }
#endif /* EXTERN_SQLITE */
#ifdef EXTERN_SQLITE
  if (execute_query(sql, NULL, 0)) {
#else /* EXTERN_SQLITE */
  puts("+----+------------+----------+----------------");
  puts("| Id |    Date    |  Amount  |  Descriptions");
  puts("+----+------------+----------+----------------");
  if (execute_query(sql, print_db_callback, 0)) {
#endif /* EXTERN_SQLITE */
    return 1;
  }
#ifndef EXTERN_SQLITE
  puts("+----+------------+----------+----------------");
#endif /* EXTERN_SQLITE */

  if (calculateSum) {
    if (execute_query("SELECT sum(amount) FROM finance;", calculate_sum_callback, 0)) {
      return 1;
    }
  }
  return 0;
}

#ifndef EXTERN_SQLITE
// TODO: add standard query output
static int standard_query_output(void* first_iter, int argc, char **argv, char **azColName) {
  if (*(int*)first_iter) {
    puts("This hasn't been implemented yet.");
    puts("It's better to define the EXTERN_SQLITE macro to use this command.");
    *(int*)first_iter = 0;
  }
  return 0;
}
#endif /* EXTERN_SQLITE */

int execute_query(char* query, int (*callback)(void*, int, char**, char**), void* relay) {
#ifdef EXTERN_SQLITE
  if (callback) {
#else /* EXTERN_SQLITE */
  if (callback == NULL) {
    callback = standard_query_output;
    int one = 1;
    relay = (void*)&one;
  }
#endif /* EXTERN_SQLITE */
    rc = sqlite3_exec(db, query, callback, relay, &zErrMsg);
    if (rc != SQLITE_OK) {
      fprintf(stderr, "SQL error(%d): %s\n", rc, zErrMsg),
      sqlite3_free(zErrMsg);
      return 1;
    }
    return 0;
#ifdef EXTERN_SQLITE
  }

  char* system_call;
  asprintf(&system_call, "sqlite3 -%s %s \"%s\"", OUTPUT_METHOD, FINANCE_DATABASE, query);
  FILE* fp = popen(system_call, "r");
  if (fp == NULL) {
    perror("Error executing sqlite3 command");
    return 1;
  }
  char line[130];
  while (fgets(line, sizeof line, fp)) {
    printf("%s", line);
  }
  return pclose(fp);
#endif /* EXTERN_SQLITE */
}

#ifdef _DEBUG
sqlite3* get_db() {
  return db;
}
#endif /* _DEBUG */