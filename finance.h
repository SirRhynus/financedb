#ifndef __FINANCE_H__
#define __FINANCE_H__

/* Opens the database file */
int open_db();

/* Closes the database file */
void close_db();

/*
Adds an entry to the database.

Args:
 - date: a date string in format compatible with SQLite
 - amount: float value representing a money amount
 - description: about the transaction

 Valid date formats are:
    YYYY-MM-DD
    now
    DDDDDDDDDD 
*/
int add(char* date, float amount, char* description);

/*
Exports the database to csv format.

Args:
 - filename: the name of the output csv file
*/
int export_to_csv(char *filename);

/*
Prints the contents of finance in stdout.

Args:
 - limit: the amount of rows to limit the output to
 - calculateSum: whether or not the sum has to be calculated over the entire amount column
*/
int print_db(int limit, int calculateSum);

/*
Executes a given query.

Args:
 - query: to be executed
 - callback: function to be executed for each row, NULL for just output printing
 - pointer to be passed on as the first argument of the callback, see sqlite3_exec()
*/
int execute_query(char* query, int (*callback)(void *, int, char **, char **), void* relay);

#ifdef _DEBUG
#include "sqlite/sqlite3.h"
/* Returns the sqlite3 database object */
sqlite3* get_db();
#endif /* _DEBUG */

#endif /* __FINCANCE_H__ */
