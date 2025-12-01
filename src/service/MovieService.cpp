#include "service/MovieService.h"
#include <stdexcept>
#include <regex>

namespace service {

MovieService::MovieService(std::shared_ptr<repository::MovieRepository> repository)
    : repository_(repository) {}

std::optional<model::Movie> MovieService::createMovie(const std::string& title, const std::vector<std::string>& genres, int duration) {
    model::Movie movie;
    movie.setTitle(title);
    movie.setGenres(genres);
    movie.setDuration(duration);
    
    // 验证影片信息
    if (!validateMovie(movie)) {
        throw std::invalid_argument("影片信息验证失败");
    }
    
    return repository_->create(movie);
}

std::optional<model::Movie> MovieService::getMovieById(int id) {
    if (id <= 0) {
        throw std::invalid_argument("无效的影片ID");
    }
    
    return repository_->findById(id);
}

MoviePageResult MovieService::getMovies(int page, int page_size, const std::string& keyword, const std::string& genre) {
    // 参数验证
    if (page < 1) {
        page = 1;
    }
    if (page_size < 1 || page_size > 100) {
        page_size = 20;
    }
    
    // 查询数据
    std::vector<model::Movie> movies = repository_->findAll(page, page_size, keyword, genre);
    int total = repository_->count(keyword, genre);
    int total_pages = (total + page_size - 1) / page_size;
    
    return {movies, total, page, page_size, total_pages};
}

bool MovieService::updateMovie(const model::Movie& movie) {
    // 验证影片信息
    if (!validateMovie(movie)) {
        throw std::invalid_argument("影片信息验证失败");
    }
    
    // 检查影片是否存在
    if (!repository_->findById(movie.getId()).has_value()) {
        throw std::invalid_argument("影片不存在");
    }
    
    return repository_->update(movie);
}

bool MovieService::deleteMovie(int id) {
    if (id <= 0) {
        throw std::invalid_argument("无效的影片ID");
    }
    
    // 检查影片是否存在
    if (!repository_->findById(id).has_value()) {
        throw std::invalid_argument("影片不存在");
    }
    
    return repository_->deleteById(id);
}

bool MovieService::validateMovie(const model::Movie& movie) {
    return validateTitle(movie.getTitle()) &&
           validateGenres(movie.getGenres()) &&
           validateDuration(movie.getDuration());
}

bool MovieService::validateTitle(const std::string& title) {
    return !title.empty() && title.length() <= 200;
}

bool MovieService::validateGenres(const std::vector<std::string>& genres) {
    if (genres.empty() || genres.size() > 5) {
        return false;
    }
    
    for (const auto& genre : genres) {
        if (genre.empty() || genre.length() > 50) {
            return false;
        }
    }
    
    return true;
}

bool MovieService::validateDuration(int duration) {
    return duration > 0 && duration <= 10000; // 最长约166小时
}

} // namespace service
