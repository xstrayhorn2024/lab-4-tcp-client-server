# Lab 4: TCP Client and Server

Operating Systems lab implementing a TCP client/server and a singly linked list in C.

## Build and run

```sh
make serv cli
./server
```

In a second terminal, run `./client`. The server listens on port 9001. Type `menu` to list commands, and `exit` to stop both programs.

## Commands

- `print`
- `get_length`
- `add_back <value>`
- `add_front <value>`
- `add_position <index> <value>`
- `remove_back`
- `remove_front`
- `remove_position <index>`
- `get <index>`
- `exit`

Positions start at 1. Insertion at length + 1 appends. Invalid removals and lookups return -1. Print output uses `value->...->NULL`.

The server owns the list and frees it on exit, disconnect, SIGINT, or SIGTERM. Signal handlers set a flag so cleanup happens in normal program flow. Replies are NUL-terminated and support large lists without truncation. The simple reference request protocol assumes one command per request, with the client waiting for a response; it does not frame arbitrarily fragmented or pipelined requests.

## Tests

```sh
python3 test_lab.py
```

Tests cover all commands, invalid input, empty lists, disconnect, Ctrl-C, the interactive client, and a 500-node list. Codio assessment score: **98/100**.
