#include "Peer.h"
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

/**
 * Constructor for the Peer class.
 * Initializes the server socket, sets the port, and starts the server loop in a separate thread.
 *
 * @param port The port number for the server socket.
 */
Peer::Peer(int port) : pool(4)
{
    WSADATA wsaData;
    PATH = "E:\\Project\\Distributed\\DistributedFileSS\\file\\test_down_file\\";
    /// Initialize Windows Sockets API
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    syncPeriod = HOURLY;

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
 * Sets the path for the peer to save downloaded data sources.
 *
 * @param path The path to save downloaded data sources.
 *
 * @return An integer indicating the success of the operation.
 *         Returns 0 if the path is empty.
 *         Returns -1 if the path does not exist.
 *         Returns 1 if the path is valid and set successfully.
 *
 * @note This function does not check if the path is writable.
 *       It is the caller's responsibility to ensure that the path is writable.
 *
 * @warning This function does not handle any errors that may occur during the path validation.
 *          It is the caller's responsibility to handle any exceptions or errors that may arise.
 *
 * @see PATH
 */
int Peer::setPeerPath(const std::string &path)
{
    // Check if the path is empty
    if (path.empty())
    {
        return 0;
    }

    // Check if the path exists
    else if (!fs::exists(path))
    {
        return -1;
    }

    // If the path is valid, set it
    else
    {
        PATH = path;
        return 1;
    }
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
    knownPeers.push_back(peerAddress);     //< Add the peer address to the list of known peers.
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
    std::lock_guard<std::mutex> lock(mtx);                                  //< Lock the mutex to ensure thread safety.
    auto it = std::find(knownPeers.begin(), knownPeers.end(), peerAddress); //< Find the position of the peer address in the list.
    if (it != knownPeers.end())                                             //< Check if the peer address was found in the list.
    {
        knownPeers.erase(it); //< Remove the peer address from the list.
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
    std::lock_guard<std::mutex> lock(mtx);      ///< Lock the mutex to ensure thread safety.
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
        if (clientSocket != INVALID_SOCKET)
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
    // ���建������СΪ4096�ֽ�
    char buffer[4096];

    // �ӿͻ��˽������ݣ����յ��ֽ����洢��received��
    int received = recv(clientSocket, buffer, sizeof(buffer), 0);

    // ������յ������ݴ���0
    if (received > 0)
    {
        // �����յ�������ת��Ϊ�ַ���
        std::string request(buffer, received);

        // ʹ��istringstream��������
        std::istringstream iss(request);

        // ����洢HTTP������·�����ַ���
        std::string method, path;

        // ����������ȡHTTP������·��
        iss >> method >> path;

        // ������󷽷���GET
        if (method == "GET")
        {
            // �����ļ�·��
            // std::string filePath = "E:/Project/Distributed/DistributedFileSS/file/test_dource_file/" + path;
            std::string filePath = path;
            // �鿴�ļ���׺��
            std::string suffix = path.substr(path.find_last_of(".") + 1);

            // E:\Project\Distributed\DistributedFileSS\file\img
            //  ���ļ����Զ�����ģʽ��ȡ
            std::ifstream file(filePath, std::ios::binary);

            // ����ļ������ҿ��Դ�
            if (file)
            {
                // ���ļ����ݶ��뻺����
                std::vector<char> fileBuffer(std::istreambuf_iterator<char>(file), {});

                // �ر��ļ�
                file.close();
                // ������Ӧͷ
                std::ostringstream oss;
               
                if (suffix != "png" && suffix != "jpg" && suffix != "jpeg" && suffix != "gif" 
                && suffix != "bmp" && suffix != "ico" && suffix != "mp4" && suffix != "mp3" 
                && suffix != "flac" && suffix != "mkv" && suffix != "svg")
                {
                    //���Ƕ������ļ��������Ӧͷ���������
                    oss << "HTTP/1.1 200 OK\r\n"
                        << "Content-Length: " << fileBuffer.size() << "\r\n"
                        << "\r\n";
                }

                // ������Ӧͷ���ͻ���
                send(clientSocket, oss.str().c_str(), oss.str().size(), 0);

                // �����ļ����ݵ��ͻ���
                send(clientSocket, fileBuffer.data(), fileBuffer.size(), 0);
            }
            else
            {
                // ����ļ������ڣ����������Ϣ
                std::cerr << "File not found: " << filePath << std::endl;

                // ����404��Ӧ���ͻ���
                std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
        }
        // ������󷽷���POST
        else if (method == "POST")
        {
            // ����501��Ӧ���ͻ��ˣ���ʾδʵ��
            std::string response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
        // ������󷽷�����GET��POST
        else
        {
            // ����400��Ӧ���ͻ��ˣ���ʾ��������
            std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }

    // �رտͻ����׽���
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
    // �ҵ���ַ��ð�ŵ�λ�ã����ڷָ�IP�Ͷ˿�
    size_t colonPos = peerAddress.find(':');

    // ��ȡIP��ַ����
    std::string address = peerAddress.substr(0, colonPos);

    // ��ȡ��ת���˿ڲ���
    int port = std::stoi(peerAddress.substr(colonPos + 1));

    // �����׽���
    SOCKET peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // ����׽��ִ���ʧ�ܣ����������Ϣ������
    if (peerSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket for peer: " << peerAddress << std::endl;
        return;
    }

    // ���岢��ʼ���Եȷ���ַ�ṹ
    sockaddr_in peerAddr;
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &peerAddr.sin_addr);

    // ���ӵ��Եȷ����������ʧ�ܣ����������Ϣ���ر��׽���
    if (connect(peerSocket, (sockaddr *)&peerAddr, sizeof(peerAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to peer: " << peerAddress << std::endl;
        closesocket(peerSocket);

        return;
    }

    // ����HTTP GET����
    std::string request = "GET " + dataSource.path + " HTTP/1.1\r\n\r\n"; // �޸���datasource.path

    // ����HTTP GET���󵽶Եȷ�
    send(peerSocket, request.c_str(), request.size(), 0);

    // ���建������СΪ4096�ֽ�
    char buffer[4096];

    // �ӶԵȷ��������ݣ����յ��ֽ����洢��received��
    int received = recv(peerSocket, buffer, sizeof(buffer), 0);

    // ������յ������ݴ���0
    if (received > 0)
    {
        // ���������ļ���·��
        // std::string filePath = "E:/Project/Distributed/DistributedFileSS/file/test_down_file/" + dataSource.name;
        std::string filePath = Peer::PATH + dataSource.name;

        // �Զ�����ģʽ���ļ�������ļ���ʧ�ܣ������½�
        std::ofstream file(filePath, std::ios::binary);
        if (file)
        {
            // �����յ�������д���ļ�
            file.write(buffer, received);

            // ������������ֱ���������
            while ((received = recv(peerSocket, buffer, sizeof(buffer), 0)) > 0)
            {
                // �����յ�������д���ļ�
                file.write(buffer, received);
            }

            // �ر��ļ�
            file.close();

            // ������سɹ���Ϣ
            std::cout << "Downloaded data source: " << dataSource.name << std::endl;
        }
        else
        {
            // ����ļ���ʧ�ܣ����Դ������ļ���д������
            std::ofstream newFile(filePath, std::ios::binary);
            if (newFile)
            {
                // �����յ�������д�����ļ�
                newFile.write(buffer, received);

                // ������������ֱ���������
                while ((received = recv(peerSocket, buffer, sizeof(buffer), 0)) > 0)
                {
                    // �����յ�������д�����ļ�
                    newFile.write(buffer, received);
                }

                // �ر����ļ� 
                newFile.close();

                // ������سɹ���Ϣ
                std::cout << "Downloaded data source: " << dataSource.name << std::endl;
            }
            else
            {
                // ����������ļ�ʧ�ܣ����������Ϣ
                std::cerr << "Failed to create or open file for writing: " << filePath << std::endl;
            }
        }
    }
    else
    {
        // �����������ʧ�ܣ����������Ϣ
        std::cerr << "Failed to receive data from peer: " << peerAddress << std::endl;
    }

    // �رնԵȷ��׽���
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
std::string Peer::displayNodeInfo()
{
    std::lock_guard<std::mutex> lock(mtx); ///< Lock the mutex to ensure thread safety.
    std::string res = "Known peers:\n";
    std::cout << "Known peers:" << std::endl;
    for (const auto &peer : knownPeers)
    {
        std::cout << " - " << peer << std::endl; ///< Display each known peer.
        res += " - " + peer + "\n";
    }
    res += "Available data sources:\n";
    std::cout << "Available data sources:" << std::endl;
    for (const auto &dataSource : availableDataSources)
    {
        std::cout << " - " << dataSource.name << std::endl; ///< Display each available data source.
        res += " - " + dataSource.name + "\n";
    }
    res += "Sync period: \n" + find_Period(Peer::syncPeriod);

    return res;
}


string find_Period(int key){
    if(key==0){
        return "Immediately";
    }else if(key==1){
        return "Minutes";
    }else if(key==2){
        return "Hours";
    }
}