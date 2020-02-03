/*
 Copyright (c) 2020 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include <gudp.h>
#ifndef WIN32
#include <arpa/inet.h>
#include <errno.h>
#else
#include <src/windows/sockets.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <gimxcommon/include/gerror.h>
#include <gimxlog/include/glog.h>

GLOG_INST(GLOG_NAME)

#define dprintf(...) \
    do { \
        if(GLOG_LEVEL(GLOG_NAME,DEBUG)) { \
            printf(__VA_ARGS__); \
            fflush(stdout); \
        } \
    } while (0)

#ifndef WIN32
#define PRINT_SOCKET_ERROR(msg) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
            fprintf(stderr, "%s:%d %s: %s failed with error: %m\n", __FILE__, __LINE__, __func__, msg); \
        } \
    } while (0)
#else
#define PRINT_SOCKET_ERROR(msg) \
    do { \
        if (GLOG_LEVEL(GLOG_NAME,ERROR)) { \
            fprintf(stderr, "%s:%d %s: %s failed with error", __FILE__, __LINE__, __func__, msg); \
            gerror_print_last(""); \
        } \
    } while (0)
#endif

struct gudp_socket {
    int fd;
    enum gudp_mode mode;
    GUDP_CALLBACKS callbacks;
    void * user;
    uint8_t buffer[1472];
};

int gudp_parse_address(const char * cp, struct gudp_address * address) {

    int ret = 0;
    int pos;
    size_t len = strlen(cp);
    //check the length
    if (len + 1 > sizeof("111.111.111.111:65535")) {
        return -1;
    }
    //check the absence of spaces
    if (strchr(cp, ' ')) {
        return -1;
    }
    char * copy = strdup(cp);
    //get the position of the ':'
    char * sep = strchr(copy, ':');
    if (sep) {
        // separate the address and the port
        *sep = ' ';
        //parse the IP
        address->ip = inet_addr(copy);
        //parse the port
        if (sscanf(sep + 1, "%hu%n", &address->port, &pos) != 1 || (unsigned int) pos != (len - (sep + 1 - copy))) {
            ret = -1;
        }
    }
    if (!sep || address->ip == INADDR_NONE || address->port == 0) {
        ret = -1;
    }
    free(copy);
    return ret;
}

char * gudp_ip_str(uint32_t ip) {

    struct in_addr addr = { .s_addr = ip };
    return inet_ntoa(addr);
}

uint32_t gudp_ntohl(uint32_t netlong) {
    return ntohl(netlong);
}

uint32_t gudp_htonl(uint32_t hostlong) {
    return htonl(hostlong);
}

struct gudp_socket * gudp_open(enum gudp_mode mode, struct gudp_address address) {

    int fd;
    int error = 0;

#ifdef WIN32
    if (wsa_init() < 0) {
        return NULL;
    }
#endif

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        PRINT_SOCKET_ERROR("socket");
        error = 1;
    }

    if (fd != -1) {
        struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = htons(address.port), .sin_addr.s_addr = address.ip };
        if (mode == GUDP_MODE_SERVER) {
            if (bind(fd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
                PRINT_SOCKET_ERROR("bind");
                error = 1;
            }
        } else if (mode == GUDP_MODE_CLIENT) {
            if (connect(fd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
                PRINT_SOCKET_ERROR("connect");
                error = 1;
            }
        }
    }

    struct gudp_socket * s = NULL;

    if (!error) {
        s = (struct gudp_socket *) calloc(1, sizeof(struct gudp_socket));
        if (s != NULL) {
            s->fd = fd;
            s->mode = mode;
        } else {
            PRINT_ERROR_ALLOC_FAILED("calloc");
            error = 1;
        }
    }

    if (error && fd >= 0) {
        close(fd);
    }

#ifdef WIN32
    wsa_count(error);
#endif

    return s;
}

int gudp_send(struct gudp_socket * socket, const void * buf, unsigned int count, struct gudp_address address) {

    if (!address.ip || !address.port) {
        PRINT_ERROR_OTHER("ip and port should not be 0");
        return -1;
    }

    struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = htons(address.port), .sin_addr.s_addr = address.ip };

    dprintf("send %d bytes to %s:%hu\n", count, gudp_ip_str(address.ip), address.port);

    int ret = sendto(socket->fd, buf, count, MSG_DONTWAIT, (struct sockaddr *) &sa, sizeof(sa));
    if (ret < 0) {
        PRINT_SOCKET_ERROR("sendto");
    }

    return ret;
}

int gudp_recv(struct gudp_socket * socket, void * buf, unsigned int count, unsigned int timeout,
        struct gudp_address * address) {

    if (address == NULL) {
        PRINT_ERROR_OTHER("address should not be NULL");
        return -1;
    }

#ifndef WIN32
    unsigned int sec = timeout / 1000;
    unsigned int usec = timeout * 1000 - sec * 1000000;
    struct timeval tv = { .tv_sec = sec, .tv_usec = usec };
    void *optval = &tv;
#else
    unsigned long tv = timeout;
    const char* optval = (char*)&tv;
#endif
    int ret = setsockopt(socket->fd, SOL_SOCKET, SO_RCVTIMEO, optval, sizeof(tv));
    if (ret < 0) {
        PRINT_SOCKET_ERROR("setsockopt SO_RCVTIMEO");
        return -1;
    }

    struct sockaddr_in sa = {};
    socklen_t salen = sizeof(sa);

    ret = recvfrom(socket->fd, buf, count, 0, (struct sockaddr *) &sa, &salen);
#ifndef WIN32
    if (ret < 0) {
        if (errno != EAGAIN) {
            PRINT_SOCKET_ERROR("recv");
        }
        return -1;
    }
#else
    if (ret < 0) {
        int error = GetLastError();
        if (error != WSAETIMEDOUT && error != WSAECONNRESET) {
          PRINT_SOCKET_ERROR("recv");
        }
        return -1;
    }
#endif

    address->ip = sa.sin_addr.s_addr;
    address->port = ntohs(sa.sin_port);

    dprintf("received %d bytes from %s:%hu\n", ret, gudp_ip_str(address->ip), address->port);

    return ret;
}

static int read_callback(void * user) {

    struct gudp_socket * socket = (struct gudp_socket *) user;

    struct gudp_address address;

    int ret = gudp_recv(socket, socket->buffer, sizeof(socket->buffer), 0, &address);

    return socket->callbacks.fp_read(socket->user, socket->buffer, ret, address);
}

static int close_callback(void * user) {

    struct gudp_socket * socket = (struct gudp_socket *) user;

    return socket->callbacks.fp_close(socket->user);
}

int gudp_register(struct gudp_socket * socket, void * user, const GUDP_CALLBACKS * callbacks) {

    if (callbacks->fp_register == NULL) {
        PRINT_ERROR_OTHER("fp_register is NULL");
        return -1;
    }

    if (callbacks->fp_remove == NULL) {
        PRINT_ERROR_OTHER("fp_remove is NULL");
        return -1;
    }

    if (callbacks->fp_read == NULL) {
        PRINT_ERROR_OTHER("fp_read is NULL");
        return -1;
    }

    GPOLL_CALLBACKS gpoll_callbacks = {
            .fp_read = read_callback,
            .fp_write = NULL,
            .fp_close = close_callback,
    };

    int ret = callbacks->fp_register(socket->fd, socket, &gpoll_callbacks);
    if (ret != -1) {
        socket->callbacks = *callbacks;
        socket->user = user;
    }

    return ret;
}

int gudp_close(struct gudp_socket * socket) {

    if (socket->fd >= 0) {
        if (socket->callbacks.fp_remove != NULL) {
            socket->callbacks.fp_remove(socket->fd);
        }
        close(socket->fd);
#ifdef WIN32
        wsa_clean();
#endif
    }

    return 0;
}
