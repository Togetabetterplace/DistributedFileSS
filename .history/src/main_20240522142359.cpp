#include <iostream>
#include <string>
#include "Peer.h"
/**
 * g++ -fdiagnostics-color=always -g E:\Project\DistributedProject\src\main.cpp E:\Project\DistributedProject\src\Peer.cpp -o E:\Project\DistributedProject\src\main.exe -lws2_32
 */

void displayMenu()
{
    std::cout << "Distributed File Synchronization and Storage System" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "1. Add a known peer: -a P" << std::endl;
    std::cout << "2. Remove a known peer: -r P" << std::endl;
    std::cout << "3. Add a data source: -a D" << std::endl;
    std::cout << "4. Remove a data source: -r D" << std::endl;
    std::cout << "5. Set synchronization period: -s Sync" << std::endl;
    std::cout << "6. " << std::endl;
    std::cout << "7. Exit" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
}

/**
 * @brief Main function of the distributed file synchronization and storage system.
 *
 * This function initializes the peer, displays the menu, and handles user input for
 * adding/removing known peers, adding/removing data sources, setting synchronization period,
 * and exiting the system.
 *
 * @return 0 upon successful execution.
 */
int main()
{
    // Initialize the peer with port 8080
    Peer peer(8080);

    // Display the menu
    displayMenu();

    // Infinite loop for user input
    while (true)
    {
        std::string command;
        std::cout << ">>> ";
        getline(std::cin,command);

        std::string peerAddress, dataSourceName, dataSourcePath, dataSourceHash;
        int syncPeriod;

        // Add a known peer
        if (command == "-a P")
        {
            std::cout << "Enter the peer address: ";
            std::cin >> peerAddress;
            peer.addKnownPeer(peerAddress);
        }
        // Remove a known peer
        else if (command == "-r P")
        {
            std::cout << "Enter the peer address: ";
            std::cin >> peerAddress;
            peer.removeKnownPeer(peerAddress);
        }
        // Add a data source
        else if (command == "-a D")
        {
            std::cout << "Enter the data source name: ";
            std::cin >> dataSourceName;
            std::cout << "Enter the data source path: ";
            std::cin >> dataSourcePath;
            std::cout << "Enter the data source hash: ";
            std::cin >> dataSourceHash;
            DataSource dataSource(dataSourceName, dataSourcePath, dataSourceHash);
            peer.addDataSource(dataSource);
        }
        // Remove a data source
        else if (command == "-r D")
        {
            std::cout << "Enter the data source name: ";
            std::cin >> dataSourceName;
            peer.removeDataSource(dataSourceName);
        }
        // Set synchronization period
        else if (command == "-s Sync")
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
                break;
            }
        }
        // Exit the system
        else if (command == "Exit")
        {
            return 0;
        }
        // Invalid choice
        else
        {
            std::cout << "Invalid choice. Please try again." << std::endl;
        }

        // Synchronize data sources
        peer.syncDataSources();

        // Sleep based on synchronization period
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
