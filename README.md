# gimxudp

The gimxudp library is a simple network communication library using UDP sockets.  
It is meant to be used for unicast streams, and it supports asynchronous operation.  
It has a compilation dependency on gimxpoll headers, on gimxlog library, and on gimxcommon source code.  

## Compilation (Linux or Msys2/MinGW64)

```
git clone https://github.com/matlo/gimxpoll.git
git clone https://github.com/matlo/gimxcommon.git
git clone https://github.com/matlo/gimxlog.git
CPPFLAGS="-I../" make -C gimxlog
git clone https://github.com/matlo/gimxudp.git
CPPFLAGS="-I../" make -C gimxudp
```

## Test sample compilation

```
CPPFLAGS="-I../" make -C gimxpoll
git clone https://github.com/matlo/gimxtime.git
CPPFLAGS="-I../" make -C gimxtime
git clone https://github.com/matlo/gimxtimer.git
CPPFLAGS="-I../" make -C gimxtimer
git clone https://github.com/matlo/gimxprio.git
CPPFLAGS="-I../" make -C gimxprio
CPPFLAGS="-I../" make -C gimxudp/test
```

## Test sample execution (Linux)

### Server

```
LD_LIBRARY_PATH=gimxpoll:gimxlog:gimxtime:gimxudp:gimxtimer:gimxprio:gimxudp gimxudp/test/gudp_test -i 127.0.0.1:51914 -s 66 -n 1000
```

### Client

```
LD_LIBRARY_PATH=gimxpoll:gimxlog:gimxtime:gimxudp:gimxtimer:gimxprio:gimxudp gimxudp/test/gudp_test -o 127.0.0.1:51914 -s 66 -n 1000 -v
```

## Test sample execution (Msys2/MinGW64)

### Server

```
PATH=gimxpoll:gimxlog:gimxtime:gimxudp:gimxtimer:gimxprio:gimxudp:$PATH gimxudp/test/gudp_test -i 127.0.0.1:51914 -s 66 -n 1000
```

### Client

```
PATH=gimxpoll:gimxlog:gimxtime:gimxudp:gimxtimer:gimxprio:gimxudp:$PATH gimxudp/test/gudp_test -o 127.0.0.1:51914 -s 66 -n 1000 -v
```
