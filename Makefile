build:
	gcc sqlite_queue.c -pthread -lsqlite3 -o sqlite-queue

build-debug:
	gcc sqlite_queue.c -pthread -lsqlite3 -o sqlite-queue -DDEBUG

format:
	astyle --style=otbs --indent=spaces=2 *.c
