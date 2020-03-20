#pragma once

#include "utils/utils.h"

// handles networking (for joining/hosting rooms on a remote sever)
class Netcp {
protected:
	void (Netcp::*tickproc)();
	Com::Buffer recvb;
	uptr<Com::Connector> connector;
	nsint socket;
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
	void tickDiscard();
};

inline void Netcp::sendData(Com::Buffer& sendb) {
	sendb.send(socket, webs);
}

inline void Netcp::sendData(const vector<uint8>& vec) {
	Com::sendData(socket, vec.data(), uint(vec.size()), webs);
}

inline void Netcp::setTickproc(void (Netcp::*func)()) {
	tickproc = func;
}

// for running one room on self as server
class NetcpHost : public Netcp {
private:
	nsint server;

public:
	NetcpHost();
	virtual ~NetcpHost() override;

	virtual void connect() override;
	virtual void disconnect() override;
	virtual void tick() override;
};

inline NetcpHost::NetcpHost() :
	server(-1)
{}
