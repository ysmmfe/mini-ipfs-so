CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

all: ipfs_add ipfs_get ipfs_daemon ipfs_fetch

ipfs_add: ipfs_add.c common.c common.h
	$(CC) $(CFLAGS) ipfs_add.c common.c -o ipfs_add -lcrypto

ipfs_get: ipfs_get.c common.c common.h
	$(CC) $(CFLAGS) ipfs_get.c common.c -o ipfs_get

ipfs_daemon: ipfs_daemon.c common.c common.h
	$(CC) $(CFLAGS) ipfs_daemon.c common.c -o ipfs_daemon

ipfs_fetch: ipfs_fetch.c common.c common.h
	$(CC) $(CFLAGS) ipfs_fetch.c common.c -o ipfs_fetch -lcrypto

test: all
	bash tests.sh

clean:
	rm -f ipfs_add ipfs_get ipfs_daemon ipfs_fetch
	rm -rf nodeA nodeB repo
