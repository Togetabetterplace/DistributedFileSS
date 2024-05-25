#include "Peer.h"
#include "DataSource.h"
#include <iostream>
#include <string>

void displayMenu()
{
    std::cout << "Distributed File Synchronization and Storage System" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "1. Add a known peer: -a P" << std::endl;
    std::cout << "2. Remove a known peer: -r P" << std::endl;
    std::cout << "3. Add a data source: -a D" << std::endl;
    std::cout << "4. Remove a data source: -r D" << std::endl;
    std::cout << "5. Set synchronization period: -s Sync" << std::endl;
    std::cout << "6. Display peer information: -d kP" << std::endl;
    std::cout << "7. Exit" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;
}

int main()
{
    Peer peer(8080);
    std::string command;
    displayMenu();
    std::cout << "Enter command (-a P, -a D, -s Sync, -d kP, Exit): "<<std::endl;
    while (true)
    {
        
        std::cout << "<<<";
        std::cin >> command;

        if (command == "-a")
        {
            std::string type;
            std::cin >> type;
            if (type == "P")
            {
                std::string peerAddress;
                std::cout << "Enter peer address: ";
                std::cin >> peerAddress;
                peer.addKnownPeer(peerAddress);
            }
            else if (type == "D")
            {
                std::string name, path, hash;
                std::cout << "Enter data source name: ";
                std::cin >> name;
                std::cout << "Enter data source path: ";
                std::cin >> path;
                std::cout << "Enter data source hash: ";
                std::cin >> hash;
                peer.addDataSource(DataSource(name, path, hash));
            }
        }
        else if (command == "-s")
        {
            std::string syncType;
            std::cin >> syncType;
            if (syncType == "Sync")
            {
                int period;
                std::cout << "Enter sync period (0: Immediate, 1: Hourly, 2: Daily): ";
                std::cin >> period;
                peer.setSyncPeriod(static_cast<SyncPeriod>(period));
            }
        }
        else if (command == "-d")
        {
            std::string type;
            std::cin >> type;
            if (type == "kP")
            {
                peer.displayNodeInfo();
            }
        }
        else if (command == "Exit")
        {
            break;
        }

        peer.syncDataSources();
    }

    return 0;
}
