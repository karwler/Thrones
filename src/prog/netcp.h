#pragma once

#include "server/server.h"

// tries to connect to a server
class Connector {
private:
	addrinfo* inf;
	addrinfo* cur;
	pollfd sock = { INVALID_SOCKET, POLLOUT, 0 };

public:
	Connector(const string& addr, const string& port, int family);
	~Connector();

	nsint pollReady();
private:
	void nextAddr(addrinfo* nxt);
};

// handles networking (for joining/hosting rooms on a remote sever)
class Netcp {
protected:
	bool (Netcp::*tickproc)() = nullptr;	// returns whether this instance was deleted
	Com::Buffer recvb;
	Program* prog;
	uptr<Connector> connector;
	pollfd sock = { INVALID_SOCKET, POLLIN | POLLRDHUP, 0 };
	bool webs = false;

public:
	Netcp(Program* program);
	virtual ~Netcp();

	virtual void connect(const Settings* sets);
	virtual void disconnect();
	virtual void tick();
	void sendData(Com::Buffer& sendb);
	void sendData(Com::Code code);
	void sendData(const vector<uint8>& vec);

	void setTickproc(bool (Netcp::*func)());
	bool tickConnect();
	bool tickWait();
	bool tickLobby();
	bool tickGame();
protected:
	bool tickValidate();
	bool tickDiscard();
	static bool pollSocket(pollfd& sock);
private:
	void sendVersionRequest();
};

inline Netcp::Netcp(Program* program) :
	prog(program)
{}

inline void Netcp::sendData(Com::Buffer& sendb) {
	sendb.send(sock.fd, webs);
}

inline void Netcp::sendData(const vector<uint8>& vec) {
	Com::sendData(sock.fd, vec.data(), vec.size(), webs);
}

inline void Netcp::setTickproc(bool (Netcp::*func)()) {
	tickproc = func;
}

// for running one room on self as server
class NetcpHost : public Netcp {
private:
	pollfd serv = { INVALID_SOCKET, POLLIN | POLLRDHUP, 0 };

public:
	using Netcp::Netcp;
	~NetcpHost() override;

	void connect(const Settings* sets) override;
	void disconnect() override;
	void tick() override;
};
