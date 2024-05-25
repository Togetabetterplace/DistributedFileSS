#include "Peer.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

Peer::Peer(int port)
{
    // Initialize the server socket
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    // Start the server thread
    std::thread serverThread(&Peer::serverLoop, this);
    serverThread.detach();
}

Peer::~Peer()
{
    closesocket(serverSocket);
    WSACleanup();
}

void Peer::addKnownPeer(const std::string &peerAddress)
{
    std::lock_guard<std::mutex> lock(mtx);
    knownPeers.push_back(peerAddress);
}

void Peer::removeKnownPeer(const std::string &peerAddress)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = std::find(knownPeers.begin(), knownPeers.end(), peerAddress);
    if (it != knownPeers.end())
    {
        knownPeers.erase(it);
    }
}

void Peer::setSyncPeriod(SyncPeriod period)
{
    syncPeriod = period;
}

SyncPeriod Peer::getSyncPeriod() const
{
    return syncPeriod;
}

void Peer::addDataSource(const DataSource &dataSource)
{
    std::lock_guard<std::mutex> lock(mtx);
    availableDataSources.push_back(dataSource);
}

void Peer::removeDataSource(const std::string &dataSourceName)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto it = std::find_if(availableDataSources.begin(), availableDataSources.end(),
                           [&dataSourceName](const DataSource &ds)
                           { return ds.name == dataSourceName; });
    if (it != availableDataSources.end())
    {
        availableDataSources.erase(it);
    }
}

void Peer::syncDataSources()
{
    std::vector<DataSource> dataSources;
    {
        std::lock_guard<std::mutex> lock(mtx);
        dataSources = availableDataSources;
    }

    for (const DataSource &dataSource : dataSources)
    {
        for (const std::string &peerAddress : knownPeers)
        {
            downloadDataSource(peerAddress, dataSource);
        }
    }
}

void Peer::serverLoop()
{
    while (true)
    {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &addrLen);

        std::thread clientThread(&Peer::handleClientConnection, this, clientSocket);
        clientThread.detach();
    }
}

void Peer::handleClientConnection(SOCKET clientSocket)
{
    // Handle the client connection, e.g., receive and process requests
    closesocket(clientSocket);
}

void Peer::downloadDataSource(const std::string &peerAddress, const DataSource &dataSource)
{
    // Connect to the peer and request the data source
    // Receive the data source from the peer and save it to local storage
}
