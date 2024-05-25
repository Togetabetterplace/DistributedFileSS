#include "Peer.h"
#include "DataSource.h"
#include <iostream>
#include <string>

int main()
{
    Peer peer(8080);
    std::string command;

    while (true)
    {
        std::cout << "Enter command (-a P, -a D, -s Sync, -d kP, Exit): ";
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
