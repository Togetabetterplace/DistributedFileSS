#include "Peer.h"
#include "DataSource.h"
#include <iostream>
#include <string>
#include "ThreadPool.h"
/
void displayMenu()
{
    std::string dragon = R"(
                                ______________
                           ,===:'.,            `-._
                                 `:.`---.__         `-._
                                   `:.     `--.         `.
                                     \.        `.         `.
                             (,,(,    \.         `.   ____,-`.,
                          (,'     `/   \.  ,--.___`.'
                     , ,' ,--.  `,   \.;'         `
                       `{D, {    \  :    \;
                         V,,'    /  /    //
                         j;;    / ,',-//.   ,---.     ,
                         \;'   / ,' /  _  \  /  _  \  ,'/
                               \   `'  / \  `'  / \  `.' /
                                `.___,'   `.__,'   `.__,'  
    
    ========================================================+
    )";
    std::cout << dragon << std::endl;

    std::string dfss = R"(
  ¨€¨€¨€¨€¨€¨€¨[    ¨€¨€¨€¨€¨€¨€¨€¨[¨€¨€¨€¨€¨€¨€¨€¨[¨€¨€¨€¨€¨€¨€¨€¨[  
  ¨€¨€¨X¨T¨€¨€¨[   ¨€¨€¨X¨T¨T¨T¨T¨a¨€¨€¨X¨T¨T¨T¨T¨a¨€¨€¨X¨T¨T¨T¨T¨a 
  ¨€¨€     ¨€¨€¨X ¨€¨€¨€¨€¨€¨[    ¨€¨€¨€¨€¨€¨€¨[  ¨€¨€¨€¨€¨€¨€¨€¨[     
  ¨€      ¨€¨€¨T¨a¨€¨€¨X¨T¨T¨a             ¨€¨€¨X ¨^¨T¨T¨T¨T¨€¨€¨U      
  ¨€¨€¨€¨€¨U     ¨€¨€¨€¨€¨[     ¨€¨€¨€¨€¨€¨€¨€¨[¨€¨€¨€¨€¨€¨€¨€¨U      
    ¨^¨T¨a     ¨^¨T¨T¨T¨T¨a     ¨^¨T¨T¨T¨T¨T¨T¨a¨^¨T¨T¨T¨T¨T¨T¨a      
========================================================+)";

    std::cout << dfss << std::endl;

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

/**
 * @brief Main function of the Distributed File Synchronization and Storage System.
 *
 * This function initializes the peer, displays the menu, and handles user input for adding/removing peers, data sources,
 * setting synchronization period, and displaying peer information. It also triggers data synchronization.
 *
 * @return 0 on successful execution.
 */
int main()
{
    /** Initialize the peer with listening port 8080 */
    Peer peer(8080);

    std::string command;

    /** Display the menu */
    displayMenu();

    std::cout << "Enter command (-a P, -a D, -s Sync, -d kP, Exit): " << std::endl;

    while (true)
    {
        std::cout << ">>>";
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

                /** Add a known peer to the peer's list of known peers */
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

                /** Add a data source to the peer's list of data sources */
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

                /** Set the synchronization period for the peer */
                peer.setSyncPeriod(static_cast<SyncPeriod>(period));
            }
        } 
        else if (command == "-d")
        {
            std::string type;
            std::cin >> type;

            if (type == "kP")
            {
                /** Display information about the peer */
                peer.displayNodeInfo();
            }
        }
        else if (command == "Exit")
        {
            break;
        }

        /** Trigger data synchronization for the peer */
        peer.syncDataSources();
    }

    return 0;
}