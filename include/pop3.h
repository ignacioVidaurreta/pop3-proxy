#ifndef POP3__H
#define POP3__H

    int servePOP3ConcurrentBlocking(const int server);

    /*
    * If the selected METHOD is X'FF', none of the methods listed by the
    * client are acceptable, and the client MUST close the connection.
    */
    static const uint8_t SOCKS_HELLO_NO_ACCEPTABLE_METHODS = 0xFF;

#endif