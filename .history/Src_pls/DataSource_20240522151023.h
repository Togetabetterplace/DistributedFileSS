#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <string>

class DataSource
{
public:
    std::string name;
    std::string path;
    std::string hash;

    DataSource(const std::string &name, const std::string &path, const std::string &hash)
        : name(name), path(path), hash(hash) {}
};

#endif // DATASOURCE_H
