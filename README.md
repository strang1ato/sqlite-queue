# sqlite-queue

**:warning: This project is still in pre-alpha stage :warning:**

`sqlite-queue` is a simple program that adds support for concurrent write queries to sqlite3

### How does it work?

Each new incoming query is safely added to a queue and then executed one by one

### Usage

Run `sqlite-queue` binary with path to sqlite3 db file, like so:
```bash
    sqlite-queue <path-to-db>
```

Each process/thread that wants to send query has to connect via [UNIX domain socket](https://man7.org/linux/man-pages/man7/unix.7.html) with `SOCK_SEQPACKET` type to: `/tmp/sqlite-queue.socket`.

In order to send query process/thread has to write to socket two text messages:
- amount of bytes(characters) in query
- and a text query

In order to close socket connection write to socket these two text messages (without quotation marks):
- "4e"
- "exit"

### Installation

Clone repository:
```bash
    git clone https://github.com/strang1ato/sqlite-queue.git
```

Make sure that you have `gcc`, `make`, `sqlite3` and `libsqlite3-dev` installed (example for debian/ubuntu):
```bash
    sudo apt-get install build-essential sqlite3 libsqlite3-dev
```

`cd` to cloned repository and run `make build`

and move `sqlite-queue` binary to directory in `$PATH`
