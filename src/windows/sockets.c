/*
 Copyright (c) 2020 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include "sockets.h"

#include <stdio.h>

//this is used to make sure WSAStartup/WSACleanup are only called once,
//and to make sure all the sockets are closed before calling WSACleanup
static unsigned int socket_count = 0;

int wsa_init() {

    WSADATA wsadata;
    if (!socket_count) {
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR) {
            fprintf(stderr, "WSAStartup failed.");
            return -1;
        }
    }
    return 0;
}

void wsa_count(int error) {

    if (!error) {
        ++socket_count;
    }

    if (!socket_count) {
        WSACleanup();
    }
}

void wsa_clean() {

    --socket_count;

    if (!socket_count) {
        WSACleanup();
    }
}
