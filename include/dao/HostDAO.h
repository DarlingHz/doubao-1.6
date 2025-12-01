#pragma once
#include <model/Host.h>
#include <dao/DbConnectionPool.h>
#include <vector>
#include <optional>

namespace dao {

class HostDAO {
public:
    explicit HostDAO(std::shared_ptr<DbConnectionPool> pool);
    
    bool createTable();
    int create(const model::Host& host);
    std::optional<model::Host> getById(int id);
    std::vector<model::Host> getAll(int limit = 100, int offset = 0);
    std::vector<model::Host> getByEnvironment(const std::string& environment, int limit = 100, int offset = 0);
    std::vector<model::Host> search(const std::string& keyword, int limit = 100, int offset = 0);
    bool update(const model::Host& host);
    bool deleteById(int id); // 软删除
    
private:
    std::shared_ptr<DbConnectionPool> pool_;
};

} // namespace dao
