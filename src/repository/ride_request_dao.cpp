// 出行请求数据访问对象实现
#include "ride_request_dao.h"
#include "db_connection.h"
#include <sstream>
#include <iomanip>

RideRequestDAO::RideRequestDAO() {
    dbManager = DBManager::getInstance();
}

RideRequestDAO::~RideRequestDAO() {
}

std::string RideRequestDAO::timePointToString(const TimePoint& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

TimePoint RideRequestDAO::stringToTimePoint(const std::string& timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

int RideRequestDAO::rideRequestStatusToInt(RideRequestStatus status) {
    switch (status) {
        case RideRequestStatus::PENDING:
            return 0;
        case RideRequestStatus::MATCHED:
            return 1;
        case RideRequestStatus::CANCELLED:
            return 2;
        case RideRequestStatus::COMPLETED:
            return 3;
        default:
            return 0;
    }
}

RideRequestStatus RideRequestDAO::intToRideRequestStatus(int statusInt) {
    switch (statusInt) {
        case 0:
            return RideRequestStatus::PENDING;
        case 1:
            return RideRequestStatus::MATCHED;
        case 2:
            return RideRequestStatus::CANCELLED;
        case 3:
            return RideRequestStatus::COMPLETED;
        default:
            return RideRequestStatus::PENDING;
    }
}

int RideRequestDAO::createRideRequest(int riderId, const Location& start, const Location& end,
                                     TimePoint earliestTime, TimePoint latestTime) {
    DBConnection* connection = dbManager->getConnection();
    std::lock_guard<std::mutex> lock(connection->getMutex());
    
    std::stringstream sql;
    sql << "INSERT INTO ride_requests (rider_id, start_x, start_y, end_x, end_y, "
        << "earliest_departure_time, latest_departure_time, creation_time) VALUES ("
        << riderId << ", " << start.x << ", " << start.y << ", " << end.x << ", " << end.y << ", \""
        << timePointToString(earliestTime) << \", \"" << timePointToString(latestTime) << \", \""
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

std::unique_ptr<RideRequest> RideRequestDAO::getRideRequestById(int id) {
    DBConnection* connection = dbManager->getConnection();
    std::unique_ptr<RideRequest> request;
    
    std::stringstream sql;
    sql << "SELECT id, rider_id, start_x, start_y, end_x, end_y, "
        << "earliest_departure_time, latest_departure_time, status, creation_time FROM ride_requests WHERE id = " << id;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return nullptr;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int requestId = sqlite3_column_int(stmt, 0);
        int riderId = sqlite3_column_int(stmt, 1);
        int startX = sqlite3_column_int(stmt, 2);
        int startY = sqlite3_column_int(stmt, 3);
        int endX = sqlite3_column_int(stmt, 4);
        int endY = sqlite3_column_int(stmt, 5);
        std::string earliestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        std::string latestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        RideRequestStatus status = intToRideRequestStatus(sqlite3_column_int(stmt, 8));
        
        TimePoint earliestTime = stringToTimePoint(earliestTimeStr);
        TimePoint latestTime = stringToTimePoint(latestTimeStr);
        
        request = std::make_unique<RideRequest>(requestId, riderId, 
                                              Location(startX, startY), Location(endX, endY),
                                              earliestTime, latestTime);
        request->setStatus(status);
    }
    
    sqlite3_finalize(stmt);
    return request;
}

bool RideRequestDAO::updateRideRequest(const RideRequest& request) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE ride_requests SET rider_id = " << request.getRiderId()
        << ", start_x = " << request.getStartLocation().x
        << ", start_y = " << request.getStartLocation().y
        << ", end_x = " << request.getEndLocation().x
        << ", end_y = " << request.getEndLocation().y
        << ", earliest_departure_time = \"" << timePointToString(request.getEarliestDepartureTime()) << "\""
        << ", latest_departure_time = \"" << timePointToString(request.getLatestDepartureTime()) << "\""
        << ", status = " << rideRequestStatusToInt(request.getStatus())
        << " WHERE id = " << request.getId();
    
    return connection->execute(sql.str());
}

bool RideRequestDAO::updateRideRequestStatus(int id, RideRequestStatus status) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE ride_requests SET status = " << rideRequestStatusToInt(status) << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

std::vector<std::unique_ptr<RideRequest>> RideRequestDAO::getPendingRideRequests() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<RideRequest>> requestList;
    
    std::stringstream sql;
    sql << "SELECT id, rider_id, start_x, start_y, end_x, end_y, "
        << "earliest_departure_time, latest_departure_time, status, creation_time FROM ride_requests WHERE status = " 
        << rideRequestStatusToInt(RideRequestStatus::PENDING);
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return requestList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int requestId = sqlite3_column_int(stmt, 0);
        int riderId = sqlite3_column_int(stmt, 1);
        int startX = sqlite3_column_int(stmt, 2);
        int startY = sqlite3_column_int(stmt, 3);
        int endX = sqlite3_column_int(stmt, 4);
        int endY = sqlite3_column_int(stmt, 5);
        std::string earliestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        std::string latestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        
        TimePoint earliestTime = stringToTimePoint(earliestTimeStr);
        TimePoint latestTime = stringToTimePoint(latestTimeStr);
        
        auto request = std::make_unique<RideRequest>(requestId, riderId, 
                                                  Location(startX, startY), Location(endX, endY),
                                                  earliestTime, latestTime);
        request->setStatus(RideRequestStatus::PENDING);
        
        // 只添加时间窗口有效的请求
        if (request->isTimeWindowValid()) {
            requestList.push_back(std::move(request));
        }
    }
    
    sqlite3_finalize(stmt);
    return requestList;
}

std::vector<std::unique_ptr<RideRequest>> RideRequestDAO::getRideRequestsByRiderId(int riderId) {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<RideRequest>> requestList;
    
    std::stringstream sql;
    sql << "SELECT id, rider_id, start_x, start_y, end_x, end_y, "
        << "earliest_departure_time, latest_departure_time, status, creation_time FROM ride_requests WHERE rider_id = " << riderId;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return requestList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int requestId = sqlite3_column_int(stmt, 0);
        int rId = sqlite3_column_int(stmt, 1);
        int startX = sqlite3_column_int(stmt, 2);
        int startY = sqlite3_column_int(stmt, 3);
        int endX = sqlite3_column_int(stmt, 4);
        int endY = sqlite3_column_int(stmt, 5);
        std::string earliestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        std::string latestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        RideRequestStatus status = intToRideRequestStatus(sqlite3_column_int(stmt, 8));
        
        TimePoint earliestTime = stringToTimePoint(earliestTimeStr);
        TimePoint latestTime = stringToTimePoint(latestTimeStr);
        
        auto request = std::make_unique<RideRequest>(requestId, rId, 
                                                  Location(startX, startY), Location(endX, endY),
                                                  earliestTime, latestTime);
        request->setStatus(status);
        
        requestList.push_back(std::move(request));
    }
    
    sqlite3_finalize(stmt);
    return requestList;
}

std::vector<std::unique_ptr<RideRequest>> RideRequestDAO::getPendingRideRequestsInRange(const Location& center, int maxDistance) {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<RideRequest>> requestList;
    
    // 使用曼哈顿距离来过滤
    int minX = center.x - maxDistance;
    int maxX = center.x + maxDistance;
    int minY = center.y - maxDistance;
    int maxY = center.y + maxDistance;
    
    std::stringstream sql;
    sql << "SELECT id, rider_id, start_x, start_y, end_x, end_y, "
        << "earliest_departure_time, latest_departure_time, status, creation_time FROM ride_requests "
        << "WHERE status = " << rideRequestStatusToInt(RideRequestStatus::PENDING) << " AND "
        << "start_x BETWEEN " << minX << " AND " << maxX << " AND "
        << "start_y BETWEEN " << minY << " AND " << maxY;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return requestList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int requestId = sqlite3_column_int(stmt, 0);
        int riderId = sqlite3_column_int(stmt, 1);
        int startX = sqlite3_column_int(stmt, 2);
        int startY = sqlite3_column_int(stmt, 3);
        int endX = sqlite3_column_int(stmt, 4);
        int endY = sqlite3_column_int(stmt, 5);
        std::string earliestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        std::string latestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        
        // 再次确认曼哈顿距离在限制内
        Location startLoc(startX, startY);
        if (startLoc.manhattanDistance(center) <= maxDistance) {
            TimePoint earliestTime = stringToTimePoint(earliestTimeStr);
            TimePoint latestTime = stringToTimePoint(latestTimeStr);
            
            auto request = std::make_unique<RideRequest>(requestId, riderId, 
                                                      Location(startX, startY), Location(endX, endY),
                                                      earliestTime, latestTime);
            request->setStatus(RideRequestStatus::PENDING);
            
            // 只添加时间窗口有效的请求
            if (request->isTimeWindowValid()) {
                requestList.push_back(std::move(request));
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return requestList;
}

std::vector<std::unique_ptr<RideRequest>> RideRequestDAO::getAllRideRequests() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<RideRequest>> requestList;
    
    std::stringstream sql;
    sql << "SELECT id, rider_id, start_x, start_y, end_x, end_y, "
        << "earliest_departure_time, latest_departure_time, status, creation_time FROM ride_requests";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return requestList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int requestId = sqlite3_column_int(stmt, 0);
        int riderId = sqlite3_column_int(stmt, 1);
        int startX = sqlite3_column_int(stmt, 2);
        int startY = sqlite3_column_int(stmt, 3);
        int endX = sqlite3_column_int(stmt, 4);
        int endY = sqlite3_column_int(stmt, 5);
        std::string earliestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        std::string latestTimeStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        RideRequestStatus status = intToRideRequestStatus(sqlite3_column_int(stmt, 8));
        
        TimePoint earliestTime = stringToTimePoint(earliestTimeStr);
        TimePoint latestTime = stringToTimePoint(latestTimeStr);
        
        auto request = std::make_unique<RideRequest>(requestId, riderId, 
                                                  Location(startX, startY), Location(endX, endY),
                                                  earliestTime, latestTime);
        request->setStatus(status);
        
        requestList.push_back(std::move(request));
    }
    
    sqlite3_finalize(stmt);
    return requestList;
}

bool RideRequestDAO::deleteRideRequest(int id) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "DELETE FROM ride_requests WHERE id = " << id;
    
    return connection->execute(sql.str());
}
