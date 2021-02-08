# Finance DB

> A SQLite database to manage your expenses.

## Requirements
- sqlite3 >= 3.33.0
- PCRE2

## Installation

### Compile sqlite3 .o file

```cc -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.o```

### Compile program

```cc -DEXERN_SQLITE -Wall finance_cli.c finance.c sqlite3.o -lpcre2-8 -o finance```

### Move to desired location (ex. /usr/local/bin)

```sudo cp ./finance /usr/local/bin```

## Usage

Add entries to your finances.
```
finance add now -2.13 "Bought batteries"
finance add 2021-01-23 +100.30 "Received payment"
```

Show finances.
```
finance show
finance show -s
finance show -l 10
```

Export to csv.
```
finance csv
```

Execute custom SQL query.
```
finance sql "SELECT date, sum(amount) FROM finance GROUP BY date;"
```

For more help.
```
finance --help <cmd>
```

# Additional info

The database is by default stored in ~/.local/share/finance/finance.db

The scheme is   
id: INTEGER | date: DATE | amount: REAL | description: TEXT
