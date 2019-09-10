#ifndef POP3__H
#define POP3__H

    int cleanUp(int fd, int originFd, int failed);
    int servePOP3ConcurrentBlocking(const int server);
    void* handleConnectionPthread(void* args);

    // const char reply_code[][50]={
	// 	{" \r\n"},  //0
	// 	{"+OK Welcome\r\n"},   //1
	// 	{"+OK\r\n"},  //2
	// 	{"-ERR bad command\r\n"},   //3
	// 	{"+OK Ready\r\n"},  //4
	// 	{"+OK Bye\r\n"},  //5
	// 	{"+OK OK\r\n"},  //6
	// 	{"+OK User not local;\r\n"},  //7
	// 	{"+OK Ready\r\n"},  //8
	// 	{"+OK Bye\r\n"},  //9
	// 	{"+OK OK\r\n"},  //10
	// 	{"+OK User not local;\r\n"},  //11
    // };

#endif