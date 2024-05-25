#include "Peer.h"
#include "DataSource.h"
#include "ThreadPool.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <condition_variable>

Peer::Peer(int port) : threadPool(4), running(true)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        exit(1);
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        exit(1);
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }

    std::thread serverThread(&Peer::serverLoop, this);
    serverThread.detach();
}

Peer::~Peer()
{
    running = false;
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
            threadPool.enqueue([this, peerAddress, dataSource]
                               { downloadDataSource(peerAddress, dataSource); });
        }
    }
}

void Peer::serverLoop()
{
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);

    while (running)
    {
        SOCKET clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &addrLen);
        if (clientSocket != INVALID_SOCKET)
        {
            std::thread(&Peer::handleClientConnection, this, clientSocket).detach();
        }
        else
        {
            std::cerr << "Failed to accept connection" << std::endl;
        }
    }
}

/**
 * @brief Handles a client connection.
 *
 * This function receives a client connection, processes the HTTP request,
 * and sends an appropriate HTTP response back to the client.
 *
 * @param clientSocket The socket descriptor for the client connection.
 * @return void
 */
void Peer::handleClientConnection(SOCKET clientSocket)
{
    char buffer[4096];
    int received = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (received > 0)
    {
        std::string request(buffer, received);
        std::istringstream iss(request);
        std::string method, path;
        iss >> method >> path;

        if (method == "GET")
        {
            std::string filePath = path; // !?
            std::ifstream file(filePath, std::ios::binary);
            if (file)
            {
                std::vector<char> fileBuffer(std::istreambuf_iterator<char>(file), {});
                file.close();
                std::ostringstream oss;
                oss << "HTTP/1.1 200 OK\r\n"
                    << "Content-Length: " << fileBuffer.size() << "\r\n"
                    << "\r\n";
                send(clientSocket, oss.str().c_str(), oss.str().size(), 0);
                send(clientSocket, fileBuffer.data(), fileBuffer.size(), 0);
            }
            else
            {
                std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
        }
        else if (method == "POST")
        {
            std::string response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
        else
        {
            std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }
    closesocket(clientSocket);
}

/**
 * @brief Displays the node information.
 *
 * This function prints out the known peers, available data sources, and synchronization period of the node.
 *
 * @return void
 */
void Peer::displayNodeInfo()
{
    std::cout << "Node Information:" << std::endl;
    std::cout << "Known Peers:" << std::endl;
    for (const std::string &peer : knownPeers)
    {
        std::cout << "\t" << peer << std::endl;
    }

    std::cout << "Available Data Sources:" << std::endl;
    for (const DataSource &dataSource : availableDataSources)
    {
        std::cout << "\t" << dataSource.name << " - " << dataSource.path << " - " << dataSource.hash << std::endl;
    }

    std::cout << "Synchronization Period: ";
    switch (syncPeriod)
    {
    case IMMEDIATE:
        std::cout << "Immediate" << std::endl;
        break;
    case HOURLY:
        std::cout << "Hourly" << std::endl;
        break;
    case DAILY:
        std::cout << "Daily" << std::endl;
        break;
    default:
        std::cout << "Unknown" << std::endl;
        break;
    }
}

/**
 * @brief Downloads a data source from a specified peer.
 *
 * This function establishes a TCP connection to the specified peer, sends a GET request for the data source,
 * and saves the received data to a local file.
 *
 * @param peerAddress The address of the peer in the format "IP:port".
 * @param dataSource The data source to be downloaded.
 */
void Peer::downloadDataSource(const std::string &peerAddress, const DataSource &dataSource)
{
    // Extract IP and port from the peer address
    std::string address = peerAddress.substr(0, peerAddress.find(':'));
    std::string port = peerAddress.substr(peerAddress.find(':') + 1);

    // Create a socket for the peer connection
    SOCKET peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (peerSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket for peer: " << peerAddress << std::endl;
        return;
    }

    // Set up the peer address structure
    sockaddr_in peerAddr;
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(std::stoi(port));
    inet_pton(AF_INET, address.c_str(), &peerAddr.sin_addr);

    // Connect to the peer
    if (connect(peerSocket, (sockaddr *)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to peer: " << peerAddress << std::endl;
        closesocket(peerSocket);
        return;
    }

    // Send a GET request for the data source
    std::string request = "GET " + dataSource.name + " HTTP/1.1\r\n\r\n";
    send(peerSocket, request.c_str(), request.size(), 0);

    // Receive the response from the peer
    char buffer[4096];
    int received = recv(peerSocket, buffer, sizeof(buffer), 0);
    if (received > 0)
    {
        // Save the received data to a local file
        std::string filePath = "E:\\Project\\DistributedProject\\flie1\\img\\" + dataSource.name;
        std::ofstream file(filePath, std::ios::binary);
        if (file)
        {
            file.write(buffer, received);
            while ((received = recv(peerSocket, buffer, sizeof(buffer), 0)) > 0)
            {
                file.write(buffer, received);
            }
            file.close();
            std::cout << "Downloaded data source: " << dataSource.name << std::endl;
        }
        else
        {
            std::cerr << "Failed to write file: " << filePath << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to receive data from peer: " << peerAddress << std::endl;
    }

    // Close the socket
    closesocket(peerSocket);
}
