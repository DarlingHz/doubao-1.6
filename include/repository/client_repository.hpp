#ifndef CLIENT_REPOSITORY_HPP
#define CLIENT_REPOSITORY_HPP

#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace api_quota {
namespace repository {

struct Client {
    uint64_t client_id = 0;
    std::string name;
    std::string contact_email;
    int64_t daily_quota = 0;
    int64_t per_minute_quota = 0;
    bool is_active = true;
    uint64_t created_at = 0;
    uint64_t updated_at = 0;
};

class ClientRepository {
public:
    ClientRepository(const std::string& database_path);
    ~ClientRepository();
    
    // 初始化数据库表
    bool init_database();
    
    // 创建新客户端
    std::optional<Client> create_client(const std::string& name, const std::string& contact_email,
                                      int64_t daily_quota, int64_t per_minute_quota);
    
    // 获取所有客户端
    std::vector<Client> get_all_clients(bool include_inactive = false);
    
    // 根据ID获取客户端
    std::optional<Client> get_client_by_id(uint64_t client_id);
    
    // 更新客户端信息
    bool update_client(uint64_t client_id, const std::optional<std::string>& name,
                      const std::optional<std::string>& contact_email,
                      const std::optional<int64_t>& daily_quota,
                      const std::optional<int64_t>& per_minute_quota,
                      const std::optional<bool>& is_active);
    
    // 删除客户端（逻辑删除）
    bool delete_client(uint64_t client_id);
    
    // 检查客户端是否存在
    bool exists(uint64_t client_id);
    
    // 获取客户端使用统计（用于报表）
    std::vector<std::pair<uint64_t, int64_t>> get_top_clients_by_daily_calls(int limit);
    
    // 获取客户端的调用摘要
    struct ClientCallSummary {
        int64_t total_calls = 0;
        int64_t rejected_calls = 0;
        std::unordered_map<std::string, int64_t> rejection_reasons;
    };
    
    std::optional<ClientCallSummary> get_client_call_summary(uint64_t client_id,
                                                           uint64_t start_time,
                                                           uint64_t end_time);
    
private:
    std::string database_path_;
    
    // SQLite相关操作（内部实现）
    class SQLiteImpl;
    std::unique_ptr<SQLiteImpl> impl_;
};

} // namespace repository
} // namespace api_quota

#endif // CLIENT_REPOSITORY_HPP