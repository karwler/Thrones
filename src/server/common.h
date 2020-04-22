#pragma once

#include "server.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define poll WSAPoll

using sendlen = int;
using socklent = int;
#else
using sendlen = ssize_t;
using socklent = socklen_t;
#endif

#ifndef POLLRDHUP
#define POLLRDHUP 0	// ignore if not present
#endif

constexpr short polleventsDisconnect = POLLERR | POLLHUP | POLLNVAL | POLLRDHUP;
