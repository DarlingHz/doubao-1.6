#ifndef MOVIE_SERVICE_H
#define MOVIE_SERVICE_H

#include "repository/MovieRepository.h"
#include "model/Movie.h"
#include <optional>
#include <string>
#include <vector>

namespace service {

struct MoviePageResult {
    std::vector<model::Movie> movies;
    int total;
    int page;
    int page_size;
    int total_pages;
};

class MovieService {
public:
    explicit MovieService(std::shared_ptr<repository::MovieRepository> repository);
    
    // 创建影片
    std::optional<model::Movie> createMovie(const std::string& title, const std::vector<std::string>& genres, int duration);
    
    // 根据ID查询影片
    std::optional<model::Movie> getMovieById(int id);
    
    // 分页查询影片
    MoviePageResult getMovies(int page, int page_size, const std::string& keyword = "", const std::string& genre = "");
    
    // 更新影片
    bool updateMovie(const model::Movie& movie);
    
    // 删除影片（逻辑删除）
    bool deleteMovie(int id);
    
    // 验证影片信息
    bool validateMovie(const model::Movie& movie);
    
private:
    std::shared_ptr<repository::MovieRepository> repository_;
    
    // 验证影片标题
    bool validateTitle(const std::string& title);
    
    // 验证影片类型
    bool validateGenres(const std::vector<std::string>& genres);
    
    // 验证影片时长
    bool validateDuration(int duration);
};

} // namespace service

#endif // MOVIE_SERVICE_H
