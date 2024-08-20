#pragma once

#ifdef LINUX
//#   define _GNU_SOURCE 1
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <unistd.h>
#	include <arpa/inet.h>
#	include <errno.h>
#	include <poll.h>
#   include <string.h>
#   include <assert.h>

namespace net {

using SocketDescriptor = int;

inline SocketDescriptor CreateNonBlockingSocket() {
    return socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
}

inline int AcceptNonBlockingSocket(SocketDescriptor svsd, struct sockaddr_in *claddr) {
    socklen_t len = sizeof(struct sockaddr_in);
    return accept4(svsd, (struct sockaddr *)claddr, &len, SOCK_NONBLOCK);
}

inline void MakeReusable(SocketDescriptor sd) {
    int so_reuseaddr = 1;
    auto result = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));
    assert(result == 0);
}

inline void CloseSocket(SocketDescriptor sd) {
    close(sd);
}

inline bool IsEWouldBlock() {
    return errno == EWOULDBLOCK;
}

inline bool IsEWouldBlockOrEInprogress() {
    return errno == EWOULDBLOCK || errno == EINPROGRESS;
}

inline bool HasSocketError(SocketDescriptor sd) {
    int err;
    socklen_t len = sizeof(err);
    auto res = getsockopt(sd, SOL_SOCKET, SO_ERROR, &err, &len);
    return res == -1 && err != 0;
}

inline const char *GetErrorString() {
    return strerror(errno);
}

inline int Poll(pollfd *fds, int n, int timeout) {
    return ::poll(fds, n, timeout);
}

}

#else
#	include <WinSock2.h>
#	include <Windows.h>
#	include <ws2def.h>
#	include <wsipv6ok.h>
#	include <WS2tcpip.h>
#	include <stdio.h>
#   include <assert.h>

namespace net {

using SocketDescriptor = SOCKET;

inline void windows_socket_nonblock(SocketDescriptor Sd) {
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(Sd, FIONBIO, &mode);
}

inline SocketDescriptor CreateNonBlockingSocket() {
    SocketDescriptor sd = socket(AF_INET, SOCK_STREAM, 0);
    windows_socket_nonblock(sd);
    return sd;
}

inline SocketDescriptor AcceptNonBlockingSocket(SocketDescriptor svsd, struct sockaddr_in *claddr) {
    int len  = sizeof(struct sockaddr_in);
    SocketDescriptor clsd = accept(svsd, (struct sockaddr *)claddr, &len);

    if (clsd != INVALID_SOCKET) {
        windows_socket_nonblock(clsd);
    }

    return clsd;
}

inline void MakeReusable(SocketDescriptor sd) {
    int so_reuseaddr = 1;
    auto result = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&so_reuseaddr, sizeof(so_reuseaddr));
    assert(result == 0);
}

inline void CloseSocket(SocketDescriptor sd) {
    closesocket(sd);
}

inline bool IsEWouldBlock() {
    return WSAGetLastError() == WSAEWOULDBLOCK;
}

inline bool IsEWouldBlockOrEInprogress() {
    return IsEWouldBlock();
}

inline bool HasSocketError(SocketDescriptor sd) {
    int err;
    int len = sizeof(err);
    return
        getsockopt(sd, SOL_SOCKET, SO_ERROR, (char*)&err, &len) != -1 &&
        err != 0;
}

inline const char *GetErrorString() {
    static char msgbuf[256] = {};

    int err = WSAGetLastError();
    FormatMessage(
       FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       err,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       msgbuf,
       sizeof(msgbuf),
       NULL);

    if (!*msgbuf) {
       snprintf(msgbuf, sizeof(msgbuf), "%d", err);
    }

    return msgbuf;
}

inline int Poll(pollfd *fds, int n, int timeout) {
    return WSAPoll(fds, n, timeout);
}

}
#endif
