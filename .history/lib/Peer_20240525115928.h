#ifndef PEER_H
#define PEER_H

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <algorithm>
#include <fstream>
#include "DataSource.h"
#include "ThreadPool.h"

enum SyncPeriod
{
    IMMEDIATE,
    HOURLY,
    DAILY
};

class Peer
{
private:
    SOCKET serverSocket;
    std::vector<std::string> knownPeers;
    std::vector<DataSource> availableDataSources;
    SyncPeriod syncPeriod;
    std::mutex mtx;
    ThreadPool pool;

    void serverLoop();
    void handleClientConnection(SOCKET clientSocket);
    void downloadDataSource(const std::string &peerAddress, const DataSource &dataSource);

public:
    Peer(int port);
    ~Peer();

    void addKnownPeer(const std::string &peerAddress);
    void removeKnownPeer(const std::string &peerAddress);

    void setSyncPeriod(SyncPeriod period);
    SyncPeriod getSyncPeriod() const;

    void addDataSource(const DataSource &dataSource);
    void removeDataSource(const std::string &dataSourceName);

    void syncDataSources();
    std::String displayNodeInfo(); // Declaration of displayNodeInfo function
};

#endif // PEER_H
