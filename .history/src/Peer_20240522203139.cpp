#include "Peer.h"
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>

/**
 * Constructor for the Peer class.
 * Initializes the server socket, binds it to the specified port, and starts the server loop in a separate thread.
 *
 * @param port The port number to bind the server socket to.
 */
Peer::Peer(int port) : pool(4)
{
    WSADATA wsaData;
    /// Initialize Windows Sockets API
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    /// Create a socket for the server
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    /// Bind the server socket to the specified port
    bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr));

    /// Listen for incoming connections on the server socket
    listen(serverSocket, 5);

    /// Start the server loop in a separate thread
    std::thread serverThread(&Peer::serverLoop, this);
    serverThread.detach();
}

/**
 * Destructor for the Peer class.
 * Closes the server socket and cleans up Windows Sockets API resources.
 */
Peer::~Peer()
{
    /// Close the server socket
    closesocket(serverSocket);

    /// Clean up Windows Sockets API resources
    WSACleanup();
}

/**
 * Adds a known peer to the list of known peers.
 *
 * @param peerAddress The address of the peer to be added.
 *
 * @return void
 */
void Peer::addKnownPeer(const std::string &peerAddress)
{
    std::lock_guard<std::mutex> lock(mtx); ///< Lock the mutex to ensure thread safety.
    knownPeers.push_back(peerAddress);       //< Add the peer address to the list of known peers.
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
            pool.enqueue([this, peerAddress, dataSource]()
                         { downloadDataSource(peerAddress, dataSource); });
        }
    }
}

void Peer::serverLoop()
{
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    while (true)
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
            std::string filePath = "E:/Project/DistributedProject/image" + path;

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
                std::cerr << "File not found: " << filePath << std::endl;
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

void Peer::downloadDataSource(const std::string &peerAddress, const DataSource &dataSource)
{
    size_t colonPos = peerAddress.find(':');
    std::string address = peerAddress.substr(0, colonPos);
    int port = std::stoi(peerAddress.substr(colonPos + 1));

    SOCKET peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (peerSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket for peer: " << peerAddress << std::endl;
        return;
    }

    sockaddr_in peerAddr;
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &peerAddr.sin_addr);

    if (connect(peerSocket, (sockaddr *)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to peer: " << peerAddress << std::endl;
        closesocket(peerSocket);
        return;
    }

    std::string request = "GET " + dataSource.name + " HTTP/1.1\r\n\r\n";
    send(peerSocket, request.c_str(), request.size(), 0);

    char buffer[4096];
    int received = recv(peerSocket, buffer, sizeof(buffer), 0);
    if (received > 0)
    {
        std::string filePath = "E:/Project/DistributedProject/image" + dataSource.name;
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
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to receive data from peer: " << peerAddress << std::endl;
    }

    closesocket(peerSocket);
}

void Peer::displayNodeInfo()
{
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "Known peers:" << std::endl;
    for (const auto &peer : knownPeers)
    {
        std::cout << " - " << peer << std::endl;
    }
    std::cout << "Available data sources:" << std::endl;
    for (const auto &dataSource : availableDataSources)
    {
        std::cout << " - " << dataSource.name << std::endl;
    }
}
