#include "repository/MovieRepository.h"
#include <sstream>
#include <iomanip>

namespace repository {

// 静态回调函数实现
int MovieRepository::findByIdCallback(void* data, int argc, char** argv, char**) {
    auto* result = static_cast<std::optional<model::Movie>*>(data);
    if (argc >= 7 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4] && argv[5] && argv[6]) {
        *result = MovieRepository::buildMovieFromRow(argv);
    }
    return 0;
}

int MovieRepository::findAllCallback(void* data, int argc, char** argv, char**) {
    auto* movies = static_cast<std::vector<model::Movie>*>(data);
    if (argc >= 7 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4] && argv[5] && argv[6]) {
        movies->push_back(MovieRepository::buildMovieFromRow(argv));
    }
    return 0;
}

int MovieRepository::findUnwatchedCallback(void* data, int argc, char** argv, char**) {
    auto* movies = static_cast<std::vector<model::Movie>*>(data);
    if (argc >= 7 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4] && argv[5] && argv[6]) {
        movies->push_back(MovieRepository::buildMovieFromRow(argv));
    }
    return 0;
}

MovieRepository::MovieRepository(DatabasePtr db) : db_(db) {}

std::optional<model::Movie> MovieRepository::create(const model::Movie& movie) {
    std::ostringstream sql;
    sql << "INSERT INTO movies (title, genres, duration) VALUES ('" 
        << movie.getTitle() << "', '" 
        << movie.genresToJsonString() << "', " 
        << movie.getDuration() << ");";
    
    if (db_->execute(sql.str())) {
        int id = static_cast<int>(db_->getLastInsertId());
        return findById(id);
    }
    
    return std::nullopt;
}

std::optional<model::Movie> MovieRepository::findById(int id) {
    std::ostringstream sql;
    sql << "SELECT id, title, genres, duration, status, created_at, updated_at FROM movies WHERE id = " << id << " AND status = 1;";
    
    std::optional<model::Movie> result;
    
    db_->query(sql.str(), findByIdCallback, &result);
    return result;
}

std::vector<model::Movie> MovieRepository::findAll(int page, int page_size, const std::string& keyword, const std::string& genre) {
    std::vector<model::Movie> movies;
    
    std::string conditions = buildQueryConditions(keyword, genre);
    int offset = (page - 1) * page_size;
    
    std::ostringstream sql;
    sql << "SELECT id, title, genres, duration, status, created_at, updated_at FROM movies WHERE status = 1 " 
        << conditions 
        << " LIMIT " << page_size << " OFFSET " << offset << ";";
    
    db_->query(sql.str(), findAllCallback, &movies);
    return movies;
}

bool MovieRepository::update(const model::Movie& movie) {
    std::ostringstream sql;
    sql << "UPDATE movies SET title = '" << movie.getTitle() 
        << "', genres = '" << movie.genresToJsonString() 
        << "', duration = " << movie.getDuration() 
        << ", updated_at = CURRENT_TIMESTAMP WHERE id = " << movie.getId() << ";";
    return db_->execute(sql.str());
}

bool MovieRepository::deleteById(int id) {
    std::ostringstream sql;
    sql << "UPDATE movies SET status = 0, updated_at = CURRENT_TIMESTAMP WHERE id = " << id << ";";
    return db_->execute(sql.str());
}

int MovieRepository::count(const std::string& keyword, const std::string& genre) {
    std::string conditions = buildQueryConditions(keyword, genre);
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM movies WHERE status = 1 " << conditions << ";";
    
    std::string result = db_->queryScalar(sql.str());
    return result.empty() ? 0 : std::stoi(result);
}

std::vector<model::Movie> MovieRepository::findUnwatchedByUserId(int user_id, int limit) {
    std::vector<model::Movie> movies;
    
    std::ostringstream sql;
    sql << "SELECT m.id, m.title, m.genres, m.duration, m.status, m.created_at, m.updated_at "
        << "FROM movies m "
        << "LEFT JOIN watch_records w ON m.id = w.movie_id AND w.user_id = " << user_id << " "
        << "WHERE m.status = 1 AND w.id IS NULL "
        << "LIMIT " << limit << ";";
    
    db_->query(sql.str(), findUnwatchedCallback, &movies);
    return movies;
}

model::Movie MovieRepository::buildMovieFromRow(char** row) {
    int id = std::stoi(row[0]);
    std::string title = row[1];
    std::string genres_json = row[2];
    int duration = std::stoi(row[3]);
    int status = std::stoi(row[4]);
    
    // 解析类型数组
    model::Movie movie;
    movie.setId(id);
    movie.setTitle(title);
    movie.genresFromJsonString(genres_json);
    movie.setDuration(duration);
    movie.setStatus(static_cast<model::Movie::Status>(status));
    
    // 解析时间字符串
    std::tm tm = {};
    std::istringstream created_at_ss(row[5]);
    created_at_ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    movie.setCreatedAt(std::mktime(&tm));
    
    std::tm tm2 = {};
    std::istringstream updated_at_ss(row[6]);
    updated_at_ss >> std::get_time(&tm2, "%Y-%m-%d %H:%M:%S");
    movie.setUpdatedAt(std::mktime(&tm2));
    
    return movie;
}

std::string MovieRepository::buildQueryConditions(const std::string& keyword, const std::string& genre) {
    std::ostringstream conditions;
    bool has_condition = false;
    
    if (!keyword.empty()) {
        conditions << " AND title LIKE '%" << keyword << "%'";
        has_condition = true;
    }
    
    if (!genre.empty()) {
        conditions << " AND genres LIKE '%" << genre << "%'";
        has_condition = true;
    }
    
    return conditions.str();
}

} // namespace repository
