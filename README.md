# Computer Network - Project 1

<h6 style="text-align: right">Wu-Jun Pei (B06902029)</h6>
### Useful Links

-   [Course Website](http://www.cmlab.csie.ntu.edu.tw/~chenyuyang/CN2019/index.html)
-   [Homework Slide](https://docs.google.com/presentation/d/1toXtMTSF8mPQRnFbcPQ5dSing_Kc_t1TgNTG2lIXqM4/edit?fbclid=IwAR2mpz8lMtRq5f7Ekw03IumdNozT0mNjLVXxCQ1PPzK-trKwGPAnVM_3ypU#slide=id.g64a74a8606_0_0)
-   [Socket Programming Tutorial](https://drive.google.com/drive/folders/1y8WdP8Q3UjaukFEr51jMZkRs35qiOlEr)


### File Hierarchy

```bash
.
├── Makefile
├── README.md
├── bin
│   ├── client
│   └── server
├── report.pdf
└── src
    ├── client.cpp
    └── server.cpp
```

-   `Makefile`
    -   `make`: To build default version.
    -   `make test`: To build debugging version with verbose.
    -   `make clean`: To remove all binaries.
-   `report.pdf`
-   `bin/`: Binaries for server and client; will be created after `make`.
    **Remember to `mkdir` it if it does not exist before `make`!**
-   `src/`: Source files (CPP) for server and client.

### Functionality

In my implementation, servers and clients first establish a TCP connection which will last until either side terminates.

For every *timeout* microseconds,

1.  The client sends a message including the current index of pings and some other information.
2.  The server echoes the message as soon as he receives it.
3.  The client waits for the messages and check if the correct index is on any of them.
    -   The client waits at most *timeout* microseconds
    -   Once successes, the client will also idle until *timeout* microseconds pass

#### Note

-   The max number of clients connected with a server is set to 256. If you want to enhance the number, go to `src/server.cpp` and change the global variable `MAX_CLIENTS`

### Stability

I've tested the both servers and clients on several linux environments including CSIE Workstations (linux3, linux7), MSLab Workstations (hp). It should work probably on those environments. :)