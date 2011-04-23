json-liner parses JSON input and generates grep friendly output. Each output
line corresponds to a JSON leaf node. json-liner is inspired by jsonpipe
(https://github.com/dvxhouse/jsonpipe).

Examples:
$ echo 123 | json-liner
/	123
$ echo '{"a": 1, "b": 2}' | json-liner
/%a	1
/%b	2
$ echo '["foo", "bar", "baz"]' | json-liner
/@0	foo
/@1	bar
/@2	baz
$ echo '{"a":{"b":1,"c":2},"d":["x",1]}' | json-liner
/%a/%b	1
/%a/%c	2
/%d/@0	x
/%d/@1	1

The default path delimiter, column delimiter, array prefix and object prefix
can be changed (even to empty). Charactor escaping is same as in JSON.

The differences between json-liner and jsonpipe are:
1. json-liner is a standalone program written in C, while jsonpipe is in python.
2. json-liner only output leaf nodes.
3. json-liner's output is not intended to reconstruct JSON.
4. Because of on-the-fly processing, json-liner uses very little memory.
