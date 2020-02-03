/*
 Copyright (c) 2020 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GUDP_H_
#define GUDP_H_

#include <stdint.h>

#include <gimxpoll/include/gpoll.h>

enum gudp_mode {
    GUDP_MODE_CLIENT,
    GUDP_MODE_SERVER
};

struct gudp_address {
    uint32_t ip;
    uint16_t port;
};

typedef int (* GUDP_READ_CALLBACK)(void * user, const void * buf, int status, struct gudp_address address);
typedef int (* GUDP_WRITE_CALLBACK)(void * user, int status);
typedef int (* GUDP_CLOSE_CALLBACK)(void * user);
typedef GPOLL_REGISTER_FD GUDP_REGISTER_SOURCE;
typedef GPOLL_REMOVE_FD GUDP_REMOVE_SOURCE;

typedef struct {
    GUDP_READ_CALLBACK fp_read;       // called on data reception
    GUDP_CLOSE_CALLBACK fp_close;     // called on failure
    GUDP_REGISTER_SOURCE fp_register; // to register the socket to event sources
    GUDP_REMOVE_SOURCE fp_remove;     // to remove the socket from event sources
} GUDP_CALLBACKS;

/*
 * \brief Structure representing a UDP socket.
 */
struct gudp_socket;

/*
 * \brief Try to parse an address with the following expected format: a.b.c.d:e
 *        where a.b.c.d is an IPv4 address and e is a port.
 *
 * \param cp       the string to parse
 * \param address  where to store the address
 *
 * \return 0 in case of success, -1 in case of error
 */
int gudp_parse_address(const char * cp, struct gudp_address * address);

/*
 * \brief Return provided ip as a string
 *
 * \param ip  the ip
 *
 * \return the ip as as string
 */
char * gudp_ip_str(uint32_t ip);

/*
 * \brief Convert an unsigned integer from network byte order to host byte order.
 *
 * \param netlong  the unsigned integer to convert
 *
 * \return the converted unsigned integer
 */
uint32_t gudp_ntohl(uint32_t netlong);

/*
 * \brief Convert an unsigned integer from host byte order to network byte order.
 *
 * \param netlong  the unsigned integer to convert
 *
 * \return the converted unsigned integer
 */
uint32_t gudp_htonl(uint32_t hostlong);

/*
 * \brief Open a UDP socket in client or server mode.
 *
 * \param mode    specifies if the UDP socket should be opened in client or server mode
 * \param address the address to bind to in server mode, or the default destination address in client mode
 *
 * \return the UDP socket
 *
 * \remark In server mode, address IP can be the IP address of a network interface, or 0.0.0.0 to mean all interfaces.
 *         In client mode, when a default destination address is set, sending to that address returns an error on
 *         second call to gudp_send if destination is not reachable.
 */
struct gudp_socket* gudp_open(enum gudp_mode mode, const struct gudp_address address);

/*
 * \brief Send data to a remote address.
 *
 * \param socket  the UDP socket
 * \param buf     the buffer containing data to send
 * \param count   the number of bytes to send
 * \param address the remote address
 *
 * \return the number of bytes sent, or -1 in case of error
 *
 * \remark Data is always sent asynchronously, which means a failure can be returned in further calls.
 *         If socket was opened in client mode with a default destination address, sending to that address returns
 *         an error on second call to gudp_send if destination is not reachable.
 */
int gudp_send(struct gudp_socket * socket, const void * buf, unsigned int count, struct gudp_address address);

/*
 * \brief Receive data from a remote host.
 *
 * \param socket  the UDP socket
 * \param buf     the buffer where to store received data
 * \param count   the maximum number of bytes to receive (buf should be large enough)
 * \param timeout the number of milliseconds to wait for (0 means no timeout / blocking)
 * \param address where to store the remote host address (should not be NULL)
 *
 * \return the number of bytes received, or -1 in case of error
 *
 * \remark Maximum returned bytes is 1472 (the classical 1500-byte MTU size minus IP and UDP headers).
 */
int gudp_recv(struct gudp_socket * socket, void * buf, unsigned int count, unsigned int timeout,
        struct gudp_address * address);
/*
 * \brief Register a UDP socket as an event source, and set the callbacks.
 *        This function triggers an asynchronous context.
 *
 * \param socket      the UDP socket
 * \param user        the user to pass to the callbacks
 * \param callbacks   the callbacks
 *
 * \return 0 in case of success, or -1 in case of error
 */
int gudp_register(struct gudp_socket * socket, void * user, const GUDP_CALLBACKS * callbacks);

/*
 * \brief Close a UDP socket.
 *
 * \param socket  the UDP socket to close
 *
 * \return 0 in case of success, or -1 in case of failure (i.e. bad UDP socket).
 */
int gudp_close(struct gudp_socket * socket);

#endif /* GUDP_H_ */
