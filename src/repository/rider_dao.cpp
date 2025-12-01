// 乘客数据访问对象实现
#include "rider_dao.h"
#include "db_connection.h"
#include <sstream>
#include <iomanip>

// 回调函数数据结构
struct RiderCallbackData {
    std::unique_ptr<Rider>* rider;
    std::vector<std::unique_ptr<Rider>>* riderList;
};

RiderDAO::RiderDAO() {
    dbManager = DBManager::getInstance();
}

RiderDAO::~RiderDAO() {
}

std::string RiderDAO::timePointToString(const TimePoint& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

TimePoint RiderDAO::stringToTimePoint(const std::string& timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// 回调函数
static int riderCallback(void* data, int argc, char** argv, char** colName) {
    RiderCallbackData* callbackData = static_cast<RiderCallbackData*>(data);
    
    int id = std::stoi(argv[0]);
    std::string name = argv[1];
    std::string phoneNumber = argv[2] ? argv[2] : "";
    float rating = std::stof(argv[3]);
    TimePoint registrationTime = callbackData->rider ? 
        callbackData->rider->get()->getRegistrationTime() : TimePoint(); // 这里简化处理
    
    if (callbackData->rider) {
        *callbackData->rider = std::make_unique<Rider>(id, name, phoneNumber);
        (*callbackData->rider)->setRating(rating);
    } else if (callbackData->riderList) {
        auto rider = std::make_unique<Rider>(id, name, phoneNumber);
        rider->setRating(rating);
        callbackData->riderList->push_back(std::move(rider));
    }
    
    return 0;
}

int RiderDAO::createRider(const std::string& name, const std::string& phoneNumber) {
    DBConnection* connection = dbManager->getConnection();
    std::lock_guard<std::mutex> lock(connection->getMutex());
    
    std::stringstream sql;
    sql << "INSERT INTO riders (name, phone_number, registration_time) VALUES (\""
        << name << \", \"" << phoneNumber << \", \""
        << timePointToString(std::chrono::system_clock::now()) << \");"
        << " SELECT last_insert_rowid();";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return -1;
    }
    
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return id;
}

std::unique_ptr<Rider> RiderDAO::getRiderById(int id) {
    DBConnection* connection = dbManager->getConnection();
    std::unique_ptr<Rider> rider;
    RiderCallbackData callbackData = {&rider, nullptr};
    
    std::stringstream sql;
    sql << "SELECT id, name, phone_number, rating, registration_time FROM riders WHERE id = " << id;
    
    connection->executeQuery(sql.str(), riderCallback, &callbackData);
    return rider;
}

bool RiderDAO::updateRider(const Rider& rider) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE riders SET name = \"" << rider.getName() 
        << \", phone_number = \"" << rider.getPhoneNumber()
        << \", rating = " << rider.getRating() 
        << " WHERE id = " << rider.getId();
    
    return connection->execute(sql.str());
}

bool RiderDAO::updateRiderRating(int id, float rating) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE riders SET rating = " << rating << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

std::vector<std::unique_ptr<Rider>> RiderDAO::getAllRiders() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Rider>> riderList;
    RiderCallbackData callbackData = {nullptr, &riderList};
    
    std::string sql = "SELECT id, name, phone_number, rating, registration_time FROM riders";
    
    connection->executeQuery(sql, riderCallback, &callbackData);
    return riderList;
}

bool RiderDAO::deleteRider(int id) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "DELETE FROM riders WHERE id = " << id;
    
    return connection->execute(sql.str());
}
