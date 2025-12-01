#ifndef MOVIE_REPOSITORY_H
#define MOVIE_REPOSITORY_H

#include "repository/Database.h"
#include "model/Movie.h"
#include <vector>
#include <optional>

namespace repository {

class MovieRepository {
public:
    explicit MovieRepository(DatabasePtr db);
    
    // 创建影片
    std::optional<model::Movie> create(const model::Movie& movie);
    
    // 根据ID查询影片
    std::optional<model::Movie> findById(int id);
    
    // 分页查询影片列表
    std::vector<model::Movie> findAll(int page, int page_size, const std::string& keyword = "", const std::string& genre = "");
    
    // 更新影片信息
    bool update(const model::Movie& movie);
    
    // 逻辑删除影片
    bool deleteById(int id);
    
    // 获取总记录数
    int count(const std::string& keyword = "", const std::string& genre = "");
    
    // 查找用户未观看过的影片
    std::vector<model::Movie> findUnwatchedByUserId(int user_id, int limit = 10);
    
private:
    DatabasePtr db_;
    
    // 从SQL结果构建影片对象
    static model::Movie buildMovieFromRow(char** row);
    
    // 静态回调函数
    static int findByIdCallback(void* data, int argc, char** argv, char**);
    static int findAllCallback(void* data, int argc, char** argv, char**);
    static int findUnwatchedCallback(void* data, int argc, char** argv, char**);
    
    // 构建查询条件
    std::string buildQueryConditions(const std::string& keyword, const std::string& genre);
};

} // namespace repository

#endif // MOVIE_REPOSITORY_H
