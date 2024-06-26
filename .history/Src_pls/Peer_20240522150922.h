#ifndef PEER_H
#define PEER_H

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "DataSource.h"
#include "ThreadPool.h"

// Define the synchronization period
enum SyncPeriod
{
    IMMEDIATE,
    HOURLY,
    DAILY
    // Add other synchronization periods
};

class Peer
{
private:
    SOCKET serverSocket;
    std::vector<std::string> knownPeers;
    std::vector<DataSource> availableDataSources;
    SyncPeriod syncPeriod;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> running;
    ThreadPool threadPool;

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
    void displayNodeInfo();
};

#endif // PEER_H
