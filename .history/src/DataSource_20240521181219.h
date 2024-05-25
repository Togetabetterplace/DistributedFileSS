#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <string>

struct DataSource
{
    std::string name;
    std::string path;
    std::string hash; // Add hash or other necessary fields

    // Constructor
    DataSource(const std::string &n, const std::string &p, const std::string &h)
        : name(n), path(p), hash(h) {}
};

#endif // DATASOURCE_H
