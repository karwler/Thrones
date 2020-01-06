#pragma once

#include "server/server.h"

// handles networking (for joining/hosting rooms on a remote sever)
class Netcp {
protected:
	SDLNet_SocketSet socks;
	TCPsocket socket;
	Com::Buffer recvb;
	void (Netcp::*cncproc)();
	bool webs;
#ifdef EMSCRIPTEN
	bool waitSend;
#endif

public:
	Netcp(uint8 maxSockets = 1);	// maxSockets shall only be altered by NetcpHost
	virtual ~Netcp();

	virtual void connect();
	virtual void disconnect();
	virtual void tick();
	void sendData(Com::Buffer& sendb);
	void sendData(Com::Code code);
	void sendData(const vector<uint8>& vec);

	void setCncproc(void (Netcp::*proc)());
	void cprocWait();
	void cprocLobby();
	void cprocGame();
protected:
	void cprocValidate();
	void cprocDiscard();

	void openSockets(const char* host, TCPsocket& sock);
	void closeSocket(TCPsocket& sock);
};

inline void Netcp::sendData(Com::Buffer& sendb) {
	sendb.send(socket, webs);
}

inline void Netcp::sendData(const vector<uint8>& vec) {
	Com::sendData(socket, vec.data(), uint(vec.size()), webs);
}

inline void Netcp::setCncproc(void (Netcp::*proc)()) {
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
	virtual void disconnect() override;
	virtual void tick() override;
};
