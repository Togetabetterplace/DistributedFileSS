#include "DataSource.h"

DataSource::DataSource(const std::string &name, const std::string &path, const std::string &hash)
{
    this->name = name;
    this->path = path;
    this->hash = hash;
}