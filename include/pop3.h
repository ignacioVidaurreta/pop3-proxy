#ifndef POP3__H
#define POP3__H

    int cleanUp(int fd, int originFd, int failed);
    int servePOP3ConcurrentBlocking(const int server);
    void* handleConnectionPthread(void* args);

#endif
