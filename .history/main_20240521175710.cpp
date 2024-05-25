#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <winsock2.h>
#include <algorithm>

// Define the data structure for data sources
struct DataSource
{
    std::string name;
    std::string path;
    // Add other necessary fields
};

// Define the synchronization period
enum SyncPeriod
{
    IMMEDIATE,
    HOURLY,
    DAILY,
    // Add other synchronization periods
};

// Define the peer class
class Peer
{
private:
    SOCKET serverSocket;
    std::vector<std::string> knownPeers;
    std::vector<DataSource> availableDataSources;
    SyncPeriod syncPeriod;
    std::mutex mtx;

public:
    Peer(int port)
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

    ~Peer()
    {
        closesocket(serverSocket);
        WSACleanup();
    }

    void addKnownPeer(const std::string &peerAddress)
    {
        mtx.lock();
        knownPeers.push_back(peerAddress);
        mtx.unlock();
    }

    void removeKnownPeer(const std::string &peerAddress)
    {
        mtx.lock();
        auto it = std::find_if(knownPeers.begin(), knownPeers.end(),
                               [&peerAddress](const std::string &p)
                               { return p == peerAddress; });
        if (it != knownPeers.end())
        {
            knownPeers.erase(it);
        }
        mtx.unlock();
    }

    void setSyncPeriod(SyncPeriod period)
    {
        syncPeriod = period;
    }

    SyncPeriod getSyncPeriod() const { return syncPeriod; }

    void addDataSource(const DataSource &dataSource)
    {
        mtx.lock();
        availableDataSources.push_back(dataSource);
        mtx.unlock();
    }

    void removeDataSource(const std::string &dataSourceName)
    {
        mtx.lock();
        auto it = std::find_if(availableDataSources.begin(), availableDataSources.end(),
                               [&dataSourceName](const DataSource &ds)
                               { return ds.name == dataSourceName; });
        if (it != availableDataSources.end())
        {
            availableDataSources.erase(it);
        }
        mtx.unlock();
    }

    void syncDataSources()
    {
        mtx.lock();
        std::vector<DataSource> dataSources = availableDataSources;
        mtx.unlock();

        for (const DataSource &dataSource : dataSources)
        {
            // Download the data source from known peers
            for (const std::string &peerAddress : knownPeers)
            {
                downloadDataSource(peerAddress, dataSource);
            }
        }
    }

private:
    void serverLoop()
    {
        while (true)
        {
            sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &addrLen);

            // Handle the client connection in a separate thread
            std::thread clientThread(&Peer::handleClientConnection, this, clientSocket);
            clientThread.detach();
        }
    }

    void handleClientConnection(SOCKET clientSocket)
    {
        // Receive data from the client and handle it
        // For example, receive a list of available data sources from the client
        // and update the local list of available data sources

        closesocket(clientSocket);
    }

    void downloadDataSource(const std::string &peerAddress, const DataSource &dataSource)
    {
        // Connect to the peer and request the data source
        // Receive the data source from the peer and save it to local storage
    }
};

// Menu for user interaction
void displayMenu()
{
    std::cout << "Distributed File Synchronization and Storage System" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "1. Add a known peer: -a P" << std::endl;
    std::cout << "2. Remove a known peer: -r P" << std::endl;
    std::cout << "3. Add a data source: -a D" << std::endl;
    std::cout << "4. Remove a data source: -r D" << std::endl;
    std::cout << "5. Set synchronization period -s period" << std::endl;
    std::cout << "6. Exit" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
}

// Main function with user interaction
int main()
{
    // Create a peer object and set up the necessary configurations
    Peer peer(8080);
    displayMenu();
    // Start the synchronization process
    while (true)
    {

        std::string command;
        std::cout << ">>> ";
        std::cin >> command;

        std::string peerAddress, dataSourceName, dataSourcePath;
        int syncPeriod;
        DataSource dataSource;
        while (command != "Exit")
        {
            if (command == "")
            {
                std::cout << "Enter the peer address: ";
                std::cin >> peerAddress;
                peer.addKnownPeer(peerAddress);
            }

            if (command == "")
            {
                std::cout << "Enter the peer address: ";
                std::cin >> peerAddress;
                peer.removeKnownPeer(peerAddress);
                break;
            }

            if (command == "")
            {
                std::cout << "Enter the data source name: ";
                std::cin >> dataSourceName;
                std::cout << "Enter the data source path: ";
                std::cin >> dataSourcePath;
                DataSource dataSource = {dataSourceName, dataSourcePath};
                peer.addDataSource(dataSource);
                break;
            }

            if (command == "")
            {
                std::cout << "Enter the data source name: ";
                std::cin >> dataSourceName;
                peer.removeDataSource(dataSourceName);
                break;
            }

            if (command == "")
            {
                std::cout << "Enter the synchronization period (0: Immediate, 1: Hourly, 2: Daily): ";
                std::cin >> syncPeriod;
                switch (syncPeriod)
                {
                case 0:
                    peer.setSyncPeriod(SyncPeriod::IMMEDIATE);
                    break;
                case 1:
                    peer.setSyncPeriod(SyncPeriod::HOURLY);
                    break;
                case 2:
                    peer.setSyncPeriod(SyncPeriod::DAILY);
                    break;
                default:
                    std::cout << "Invalid synchronization period." << std::endl;
                }
                break;
            }

            if (command == "")
            {
                return 0;
            }
            else
            {
                std::cout << "Invalid choice. Please try again." << std::endl;
            }

            peer.syncDataSources();

            // Wait for the synchronization period
            if (peer.getSyncPeriod() == SyncPeriod::IMMEDIATE)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
            }
            else if (peer.getSyncPeriod() == SyncPeriod::HOURLY)
            {
                std::this_thread::sleep_for(std::chrono::hours(1));
            }
            else if (peer.getSyncPeriod() == SyncPeriod::DAILY)
            {
                std::this_thread::sleep_for(std::chrono::hours(24));
            }
        }

        return 0;
    }
}