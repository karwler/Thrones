#pragma once

#include "server/server.h"

class NetcpHost;

struct NetcpException {
	const string message;

	NetcpException(string&& msg);
};

inline NetcpException::NetcpException(string&& msg) :
	message(std::move(msg))
{}

// handles networking (for joining/hosting rooms on a remote sever)
class Netcp {
protected:
	SDLNet_SocketSet socks;
	TCPsocket socket;
	Buffer recvb;
	void (*cncproc)(uint8*);

public:
	Netcp(uint8 maxSockets = 1);	// maxSockets shall only be altered by NetcpHost
	virtual ~Netcp();

	virtual void connect();
	virtual void tick();
	void sendData(Buffer& sendb);
	template <class T> void sendData(const vector<T>& vec);
	void sendData(Com::Code code);

	static void cprocDiscard(uint8* data);
	static void cprocWait(uint8* data);
	static void cprocLobby(uint8* data);
	static void cprocGame(uint8* data);
	void setCncproc(void (*proc)(uint8*));

protected:
	void openSockets(const char* host, TCPsocket& sock);
	void closeSocket(TCPsocket& sock);
	void checkSocket();
};

template <class T>
void Netcp::sendData(const vector<T>& vec) {
	if (uint16 len = uint16(vec.size() * sizeof(T)); SDLNet_TCP_Send(socket, vec.data(), len) != len)
		throw NetcpException(SDLNet_GetError());
}

inline void Netcp::cprocDiscard(uint8* data) {
	std::cerr << "unexprected data with code " << uint(data[0]) << std::endl;
}

inline void Netcp::setCncproc(void (*proc)(uint8*)) {
	cncproc = proc;
}

// for running one room on self as server
class NetcpHost : public Netcp {
private:
	TCPsocket server;

public:
	NetcpHost();
	virtual ~NetcpHost() override;

	virtual void connect() override;
	virtual void tick() override;
};
