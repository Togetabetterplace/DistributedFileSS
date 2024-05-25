#include "Peer.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>

/**
 * @brief Constructor for the Peer class. Initializes the server socket and starts the server thread.
 *
 * @param port The port number to listen for incoming connections.
 */
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

/**
 * @brief Destructor for the Peer class.
 *
 * Closes the server socket and cleans up the Windows Sockets API.
 * This ensures proper resource management when the Peer object is destroyed.
 */
Peer::~Peer()
{
    closesocket(serverSocket);  // Close the server socket
    WSACleanup();  // Clean up the Windows Sockets API
}

/**
 * @brief Adds a known peer to the list of known peers.
 *
 * This function is thread-safe and ensures that the known peers list is updated
 * correctly even when multiple threads are accessing it simultaneously.
 *
 * @param peerAddress The address of the known peer to be added.
 */
void Peer::addKnownPeer(const std::string &peerAddress)
{
    std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to ensure thread safety
    knownPeers.push_back(peerAddress);  // Add the peer address to the known peers list
}

/**
 * @brief Removes a known peer from the list of known peers.
 *
 * This function is thread-safe and ensures that the known peers list is updated
 * correctly even when multiple threads are accessing it simultaneously.
 *
 * @param peerAddress The address of the known peer to be removed.
 *
 * @return void
 */
void Peer::removeKnownPeer(const std::string &peerAddress)
{
    std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to ensure thread safety
    auto it = std::find(knownPeers.begin(), knownPeers.end(), peerAddress);
    if (it!= knownPeers.end())
    {
        knownPeers.erase(it);
    }
}

/**
 * @brief Sets the synchronization period for data sources.
 *
 * This function allows the user to set the synchronization period for data sources.
 * The synchronization period determines how often the data sources will be synchronized
 * with the known peers.
 *
 * @param period The synchronization period to be set.
 *
 * @return void
 */
void Peer::setSyncPeriod(SyncPeriod period)
{
    syncPeriod = period;
}

/**
 * @brief Gets the synchronization period for data sources.
 *
 * This function retrieves the synchronization period for data sources.
 * The synchronization period determines how often the data sources will be synchronized
 * with the known peers.
 *
 * @return The synchronization period.
 */
SyncPeriod Peer::getSyncPeriod() const
{
    return syncPeriod;
}

/**
 * @brief Adds a new data source to the list of available data sources.
 *
 * This function is thread-safe and ensures that the available data sources list is updated
 * correctly even when multiple threads are accessing it simultaneously.
 *
 * @param dataSource The data source to be added.
 *
 * @return void
 */
void Peer::addDataSource(const DataSource &dataSource)
{
    std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to ensure thread safety
    availableDataSources.push_back(dataSource);  // Add the data source to the available data sources list
}

/**
 * @brief Removes a data source from the list of available data sources.
 *
 * This function is thread-safe and ensures that the available data sources list is updated
 * correctly even when multiple threads are accessing it simultaneously.
 *
 * @param dataSourceName The name of the data source to be removed.
 *
 * @return void
 */
void Peer::removeDataSource(const std::string &dataSourceName)
{
    std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to ensure thread safety
    auto it = std::find_if(availableDataSources.begin(), availableDataSources.end(),
                           [&dataSourceName](const DataSource &ds)
                           { return ds.name == dataSourceName; });
    if (it!= availableDataSources.end())
    {
        availableDataSources.erase(it);
    }
}

/**
 * @brief Synchronizes the data sources with the known peers.
 *
 * This function retrieves the list of available data sources and known peers,
 * then iterates through each data source and known peer to initiate a download.
 * The function ensures thread safety by locking the mutex before accessing the
 * available data sources and known peers lists.
 *
 * @return void
 */
void Peer::syncDataSources()
{
    std::vector<DataSource> dataSources;
    {
        std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to ensure thread safety
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
    // Split the peerAddress into IP and port
    size_t colonPos = peerAddress.find(':');
    if (colonPos == std::string::npos)
    {
        std::cerr << "Invalid peer address format: " << peerAddress << std::endl;
        return;
    }
    std::string peerIP = peerAddress.substr(0, colonPos);
    int peerPort = std::stoi(peerAddress.substr(colonPos + 1));

    // Create a socket
    SOCKET peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (peerSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

    // Set up the sockaddr_in structure for the peer
    sockaddr_in peerAddr;
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(peerPort);
    peerAddr.sin_addr.s_addr = inet_addr(peerIP.c_str());

    // Connect to the peer
    if (connect(peerSocket, (sockaddr *)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to peer: " << peerAddress << std::endl;
        closesocket(peerSocket);
        return;
    }

    // Send the request for the data source
    std::string request = "GET " + dataSource.name + "\n";
    if (send(peerSocket, request.c_str(), request.length(), 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send request to peer: " << peerAddress << std::endl;
        closesocket(peerSocket);
        return;
    }

    // Receive the data source
    const int bufferSize = 4096;
    char buffer[bufferSize];
    std::string receivedData;
    int bytesReceived;
    while ((bytesReceived = recv(peerSocket, buffer, bufferSize, 0)) > 0)
    {
        receivedData.append(buffer, bytesReceived);
    }

    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr << "Error receiving data from peer: " << peerAddress << std::endl;
        closesocket(peerSocket);
        return;
    }

    // Save the received data to local storage
    std::ofstream outFile(dataSource.path, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open file for writing: " << dataSource.path << std::endl;
    }
    else
    {
        outFile.write(receivedData.c_str(), receivedData.size());
        outFile.close();
    }

    // Clean up
    closesocket(peerSocket);
}
