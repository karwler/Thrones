#pragma once

#include "server/server.h"

// tries to connect to a server
class Connector {
private:
	addrinfo* inf;
	addrinfo* cur;
	pollfd sock;

public:
	Connector(const char* addr, const char* port, int family);
	~Connector();

	nsint pollReady();
private:
	void nextAddr(addrinfo* nxt);
};

// handles networking (for joining/hosting rooms on a remote sever)
class Netcp {
protected:
	void (Netcp::*tickproc)();
	Com::Buffer recvb;
	uptr<Connector> connector;
	pollfd sock;
	bool webs;

public:
	Netcp();
	virtual ~Netcp();

	virtual void connect();
	virtual void disconnect();
	virtual void tick();
	void sendData(Com::Buffer& sendb);
	void sendData(Com::Code code);
	void sendData(const vector<uint8>& vec);

	void setTickproc(void (Netcp::*func)());
	void tickConnect();
	void tickWait();
	void tickLobby();
	void tickGame();
protected:
	void tickValidate();
	void tickDiscard() {}
	static bool pollSocket(pollfd& sock);
};

inline void Netcp::sendData(Com::Buffer& sendb) {
	sendb.send(sock.fd, webs);
}

inline void Netcp::sendData(const vector<uint8>& vec) {
	Com::sendData(sock.fd, vec.data(), uint(vec.size()), webs);
}

inline void Netcp::setTickproc(void (Netcp::*func)()) {
	tickproc = func;
}

// for running one room on self as server
class NetcpHost : public Netcp {
private:
	pollfd serv;

public:
	NetcpHost();
	virtual ~NetcpHost() override;

	virtual void connect() override;
	virtual void disconnect() override;
	virtual void tick() override;
};

inline NetcpHost::NetcpHost() :
	serv{ INVALID_SOCKET, POLLIN | POLLRDHUP, 0 }
{}
