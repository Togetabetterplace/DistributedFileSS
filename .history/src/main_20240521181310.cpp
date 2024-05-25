#include <iostream>
#include <string>
#include "Peer.h"

void displayMenu()
{
    std::cout << "Distributed File Synchronization and Storage System" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "1. Add a known peer: -a P" << std::endl;
    std::cout << "2. Remove a known peer: -r P" << std::endl;
    std::cout << "3. Add a data source: -a D" << std::endl;
    std::cout << "4. Remove a data source: -r D" << std::endl;
    std::cout << "5. Set synchronization period: -s Sync" << std::endl;
    std::cout << "6. Exit" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
}

int main()
{
    Peer peer(8080);
    displayMenu();

    while (true)
    {
        std::string command;
        std::cout << ">>> ";
        std::cin >> command;

        std::string peerAddress, dataSourceName, dataSourcePath, dataSourceHash;
        int syncPeriod;

        if (command == "-a P")
        {
            std::cout << "Enter the peer address: ";
            std::cin >> peerAddress;
            peer.addKnownPeer(peerAddress);
        }
        else if (command == "-r P")
        {
            std::cout << "Enter the peer address: ";
            std::cin >> peerAddress;
            peer.removeKnownPeer(peerAddress);
        }
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
        else if (command == "-r D")
        {
            std::cout << "Enter the data source name: ";
            std::cin >> dataSourceName;
            peer.removeDataSource(dataSourceName);
        }
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
        else if (command == "Exit")
        {
            break;
        }
        else
        {
            std::cout << "Invalid choice. Please try again." << std::endl;
        }

        peer.syncDataSources();

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
