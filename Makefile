build:
	gcc sqlite_queue.c -pthread -lsqlite3 -o sqlite-queue

format:
	astyle --style=otbs --indent=spaces=2 *.c
