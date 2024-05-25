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
    std::lock_guard<std::mutex> lock(mtx); //< Lock the mutex to ensure thread safety.
    knownPeers.push_back(peerAddress);       //< Add the peer address to the list of known peers.
}

/**
 * Removes a known peer from the list of known peers.
 *
 * @param peerAddress The address of the peer to be removed.
 *
 * @return void
 *
 * @note This function is thread-safe and should be called from a thread that has access to the shared data.
 *
 * @warning This function does not check if the peer address exists in the list before attempting to remove it.
 *          It is the caller's responsibility to ensure that the peer address is valid and exists in the list.
 *
 * @see addKnownPeer()
 * @see knownPeers
 */
void Peer::removeKnownPeer(const std::string &peerAddress)
{
    std::lock_guard<std::mutex> lock(mtx);                                        //< Lock the mutex to ensure thread safety.
    auto it = std::find(knownPeers.begin(), knownPeers.end(), peerAddress);       //< Find the position of the peer address in the list.
    if (it!= knownPeers.end())                                                    //< Check if the peer address was found in the list.
    {
        knownPeers.erase(it);                                                     //< Remove the peer address from the list.
    }
}

/**
 * Sets the synchronization period for the peer.
 *
 * @param period The synchronization period to be set.
 *
 * @return void
 *
 * @note This function does not perform any synchronization operations.
 *       It only updates the internal state of the peer with the specified synchronization period.
 *
 * @warning The synchronization period should be a positive integer value.
 *          If a negative value or zero is provided, the behavior of the peer is undefined.
 *
 * @see getSyncPeriod()
 * @see syncDataSources()
 */
void Peer::setSyncPeriod(SyncPeriod period)
{
    syncPeriod = period;
}

/**
 * Retrieves the synchronization period for the peer.
 *
 * @return The synchronization period.
 *
 * @note This function does not perform any synchronization operations.
 *       It only retrieves the internal state of the peer with the specified synchronization period.
 *
 * @warning The synchronization period should be a positive integer value.
 *          If a negative value or zero is provided, the behavior of the peer is undefined.
 *
 * @see setSyncPeriod()
 * @see syncDataSources()
 */
SyncPeriod Peer::getSyncPeriod() const
{
    return syncPeriod;
}

/**
 * Adds a data source to the list of available data sources.
 *
 * @param dataSource The data source to be added.
 *
 * @return void
 *
 * @note This function is thread-safe and should be called from a thread that has access to the shared data.
 *
 * @warning This function does not check if the data source already exists in the list before attempting to add it.
 *          It is the caller's responsibility to ensure that the data source is unique and does not exist in the list.
 *
 * @see removeDataSource()
 * @see availableDataSources
 */
void Peer::addDataSource(const DataSource &dataSource)
{
    std::lock_guard<std::mutex> lock(mtx); ///< Lock the mutex to ensure thread safety.
    availableDataSources.push_back(dataSource); ///< Add the data source to the list of available data sources.
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

/**
 * Synchronizes the data sources with the known peers.
 * This function retrieves the list of available data sources and known peers,
 * then enqueues a download task for each combination of data source and peer.
 * The download tasks are executed concurrently using a thread pool.
 *
 * @return void
 *
 * @note This function is thread-safe and should be called from a thread that has access to the shared data.
 *
 * @warning This function does not handle any errors that may occur during the download process.
 *          It is the caller's responsibility to handle any exceptions or errors that may arise.
 *
 * @see downloadDataSource()
 * @see knownPeers
 * @see availableDataSources
 * @see ThreadPool
 */
void Peer::syncDataSources()
{
    std::vector<DataSource> dataSources;
    {
        std::lock_guard<std::mutex> lock(mtx); ///< Lock the mutex to ensure thread safety.
        dataSources = availableDataSources;    ///< Copy the list of available data sources.
    }

    for (const DataSource &dataSource : dataSources)
    {
        for (const std::string &peerAddress : knownPeers)
        {
            /// Enqueue a download task for each combination of data source and peer.
            pool.enqueue([this, peerAddress, dataSource]()
                         { downloadDataSource(peerAddress, dataSource); });
        }
    }
}

/**
 * The server loop for the peer.
 * This function continuously listens for incoming connections on the server socket.
 * When a connection is accepted, a new thread is created to handle the client connection.
 *
 * @return void
 *
 * @note This function runs in an infinite loop until the program is terminated.
 *
 * @warning This function does not handle any errors that may occur during the accept operation.
 *          It is the caller's responsibility to handle any exceptions or errors that may arise.
 *
 * @see handleClientConnection()
 * @see serverSocket
 */
void Peer::serverLoop()
{
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    while (true)
    {
        SOCKET clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &addrLen);
        if (clientSocket!= INVALID_SOCKET)
        {
            /// Detach a new thread to handle the client connection.
            std::thread(&Peer::handleClientConnection, this, clientSocket).detach();
        }
        else
        {
            std::cerr << "Failed to accept connection" << std::endl;
        }
    }
}

/**
 * Handles a client connection by receiving a request, processing it, and sending a response.
 *
 * @param clientSocket The socket descriptor for the client connection.
 *
 * @return void
 *
 * @note This function is called in a separate thread for each client connection.
 *
 * @warning This function does not handle any errors that may occur during the request processing or response sending.
 *          It is the caller's responsibility to handle any exceptions or errors that may arise.
 *
 * @see serverLoop()
 * @see clientSocket
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

/**
 * Downloads a data source from a specified peer.
 *
 * @param peerAddress The address of the peer to download the data source from.
 * @param dataSource  The data source to be downloaded.
 *
 * @return void
 *
 * @note This function establishes a TCP connection to the specified peer, sends a GET request for the data source,
 *       and saves the received data to a local file.
 *
 * @warning This function does not handle any errors that may occur during the download process.
 *          It is the caller's responsibility to handle any exceptions or errors that may arise.
 *
 * @see syncDataSources()
 * @see knownPeers
 * @see availableDataSources
 */
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

/**
 * Displays the information about the node, including known peers and available data sources.
 *
 * @return void
 *
 * @note This function is thread-safe and should be called from a thread that has access to the shared data.
 *
 * @warning This function does not handle any errors that may occur during the display process.
 *          It is the caller's responsibility to handle any exceptions or errors that may arise.
 *
 * @see knownPeers
 * @see availableDataSources
 */
void Peer::displayNodeInfo()
{
    std::lock_guard<std::mutex> lock(mtx); ///< Lock the mutex to ensure thread safety.

    std::cout << "Known peers:" << std::endl;
    for (const auto &peer : knownPeers)
    {
        std::cout << " - " << peer << std::endl; ///< Display each known peer.
    }

    std::cout << "Available data sources:" << std::endl;
    for (const auto &dataSource : availableDataSources)
    {
        std::cout << " - " << dataSource.name << std::endl; ///< Display each available data source.
    }
}
