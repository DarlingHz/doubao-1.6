#ifndef DATABASE_CONFIG_H
#define DATABASE_CONFIG_H

#include <string>

namespace db {

struct DatabaseConfig {
    std::string host = "localhost";
    int port = 3306;
    std::string database = "personal_knowledge_cards";
    std::string username = "root";
    std::string password = "";
    int connectionPoolSize = 10;
};

} // namespace db

#endif // DATABASE_CONFIG_H