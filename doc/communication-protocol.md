# Spawner Communication Protocol --- SCP/8

# Abstract

Specification of communication between programs being concurrently sandboxed by Spawner.

# Requirements

The key words `MUST`, `MUST NOT`, `REQUIRED`, `SHALL`, `SHALL NOT`, `SHOULD`,
`SHOULD NOT`, `RECOMMENDED`, `MAY`, and `OPTIONAL` in this document are to be
interpreted as described in [RFC 2119](https://tools.ietf.org/html/rfc2119).

An implementation is not compliant if it fails to satisfy one or more of the
MUST or REQUIRED level requirements for the protocols it implements. An
implementation that satisfies all the MUST or REQUIRED level and all the SHOULD
level requirements for its protocols is said to be `unconditionally compliant`;
one that satisfies all the MUST level requirements but not all the SHOULD level
requirements for its protocols is said to be `conditionally compliant.`

# Introduction

# Terminology

__Spawner__ is a program for (possibly) concurrent sandboxing of arbitrary applications.

__Controller__ is a spawner-sandboxed program. __Controller__ is aware of each __normal__ existence and able to affect their regulation by spawner. There MUST be only one __controller__ program.

__Normal__ is a spawner-sandboxed program. __Normal__ is only aware of __controller__. There MAY be many __normal__ programs.

__Message__ is an atomic unit of communication between __normal__ and __controller__ or spawner and __controller__.

__Control mode__ is a spawner mode being activated when there is program marked as __controller__.

# Running Scheme

Both __normal__ and __controller__ are run with spawner. In a single spawner invocation with __controller__ and __normal__ being passed via command line arguments.
- The __controller__ MUST be marked with `--controller` flag.
- The __controller__ MUST be the only one.
- The presence of __controller__ MUST trigger spawner into control mode.
- Rest of the requirements MUST only be applied in control mode.
- The number of __normals__ MUST be passed as a first command line argument to __controller__.
- Each __normal__'s stdin MUST be connected with __controller__'s stdout with `--in=*0.stdout`.
- Each __normal__'s stdout MUST be connected with __controller__'s stdin with `--out=*0.stdout`

Sample command line:

```
sp.exe --json -sr=report.json -hr=1 --separator=//
--// --controller --out=std controller.exe 3
--// --out=std --in=*0.stdout --out=*0.stdin normal-1.exe
--// --out=std --in=*0.stdout --out=*0.stdin normal-2.exe
--// --out=std --in=*0.stdout --out=*0.stdin normal-3.exe
```

# Communication Rules

- all __normal__ programs MUST start in suspended mode.
- spawner MUST resume __normal__ in response to wait request from __controller__ to wait for __normal__ with given index.
- spawner MUST put awaited __normal__ process into suspended state after it responded with a message.

- __controller__ MAY send a message to specified __normal__
- __controller__ MAY send a message to the spawner, informing it about waiting for specific or any __normal__ to send message to __controller__
- __normal__ MAY send a message to the __controller__
- __controller__ MAY ask spawner to stop specified __normal__ process
- __controller__'s deadline MUST be reset after each message sent by __controller__
- __normal__'s deadline MUST be set if it's being awaited by __controller__
- __normal__'s deadline MUST be reset if __normal__ sends a message

>TODO: add examples

# Messages

In general messages have a form : `<header><body>\n`. `<body>` MAY contain any character but `\n` and `\r`. `<header>` MUST always end with `#` symbol. `#` MAY be preceded by a letter which MAY be preceded by integer. e.g. `5S#Hello, world!\n`.

- __Controller__ and __normal__s MUST send messages via `stdout`
- `stdout` MUST be flushed after sending a message. e.g. `fflush(stdout);` in C++.
- Messages MUST be separated by newline (`\n`). e.g.

## Examples

This MUST produce two messages `First message\n` and `Second message\n` (note included `\n`s):
```C++
printf("First message\nSecond message\n");
fflush(stdout);
```

while this MUST produce one message (`\n`):
```C++
printf("\n");
fflush(stdout);
```

and this one will produces no message since there is no newline present:
```C++
printf("foo");
fflush(stdout);
```

while this will produce undefined behavior since stdout is not being flushed:
```C++
printf("foo\n");
```

## Controller messages

__Controller__ MAY post two kinds of a message:

### A message to the __normal__

__Controller__ MUST prefix message text with a number followed by `#` if intention is to send a message to __normal__. This number stands for __normal__ index. Normals are indexed from 1 to n where n is total number of __normal__s in order they're passed to spawner via command line arguments.

Considering this command line:

```
sp.exe --separator=//
--// --controller controller.exe 3
--// --in=*0.stdout --out=*0.stdin normal-1.exe
--// --in=*0.stdout --out=*0.stdin normal-2.exe
--// --in=*0.stdout --out=*0.stdin normal-3.exe
```

In order to send a message `Foo` to application `normal-1.exe` __controller__ must:
```C++
printf("1#Foo\n");
fflush(stdout);
```

If __controller__ specifies an invalid index then spawner MUST send the following error message to the __controller__: `<i>I#\n` where `<i>` is an index of invalid __normal__.

__Controller__ MAY specify if it awaits a response from that __normal__ or any __normal__. Time limit MUST NOT be issued against a __normal__ which response is not awaited for.

To specify waiting for arbitrary __normal__: `<i>W#\n`.
To specify waiting for any __normal__: `W#\n` (not implemented yet)

### A message to the spawner

__Controller__ MAY ask spawner to stop executing specified __normal__ with:

- `<i>S` spawner MUST shut down the __normal__ with index `<i>`

Spawner MUST not reply to this message if `<i>` is invalid.

e.g.
```C++
printf('4S#\n');
fflush(stdout);
```

Messages sent to 0 index are considered being sent to spawner an OPTIONAL letter may clarify the message. This case is reserved for further versions of protocol.

## Normal messages

Spawner MUST prefix each __normal__ message with __normal__ index followed by `#` symbol e.g.
```C++
// let this normal's index be 3
printf("12 45 56 65\n");
fflush(stdout);
// controller will get "3#12 45 56 65\n"
```
