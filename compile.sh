#!/bin/bash

gcc -pthread -std=c99 -pedantic -Wall main.c pop3.c parser.c server.c client.c config.c -o run

