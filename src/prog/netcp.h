#pragma once

#include "server/server.h"

// handles networking
class Netcp {
public:
	uint16 sendSize;	// shall not exceed data size, is never actually checked though
	uint8 sendb[Com::recvSize];
protected:
	Com::Code recvState;
	uint16 recvPos;
	uint8 recvb[Com::recvSize];

	SDLNet_SocketSet socks;
	TCPsocket socket;

public:
	Netcp();
	virtual ~Netcp();

	virtual void connect();
	virtual void tick();
	bool sendData();

	template <class T> void push(T val);
	template <class T> void push(const vector<T>& vec);
protected:
	void openSockets(const char* host, TCPsocket& sock, uint8 num);	// throws const char* error message
	void closeSocket(TCPsocket& sock);
	void closeSockets(TCPsocket& last);
	void checkSocket();
};

template <class T>
inline void Netcp::push(T val) {
	std::copy_n(reinterpret_cast<const uint8*>(&val), sizeof(val), sendb + sendSize);
	sendSize += sizeof(val);
}

template <class T>
void Netcp::push(const vector<T>& vec) {
	uint16 len = uint16(vec.size() * sizeof(T));
	std::copy_n(reinterpret_cast<const uint8*>(vec.data()), len, sendb + sendSize);
	sendSize += len;
}

class NetcpHost : public Netcp {
private:
	TCPsocket server;
	void (NetcpHost::*tickfunc)();

public:
	NetcpHost();
	virtual ~NetcpHost() override;

	virtual void connect() override;
	virtual void tick() override;
private:
	void tickWait();
	void tickGame();
};
