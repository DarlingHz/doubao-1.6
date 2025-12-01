// 行程数据访问对象实现
#include "trip_dao.h"
#include "db_connection.h"
#include <sstream>
#include <iomanip>

TripDAO::TripDAO() {
    dbManager = DBManager::getInstance();
}

TripDAO::~TripDAO() {
}

std::string TripDAO::timePointToString(const TimePoint& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

TimePoint TripDAO::stringToTimePoint(const std::string& timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

int TripDAO::tripStatusToInt(TripStatus status) {
    switch (status) {
        case TripStatus::ONGOING:
            return 0;
        case TripStatus::COMPLETED:
            return 1;
        case TripStatus::CANCELLED:
            return 2;
        default:
            return 0;
    }
}

TripStatus TripDAO::intToTripStatus(int statusInt) {
    switch (statusInt) {
        case 0:
            return TripStatus::ONGOING;
        case 1:
            return TripStatus::COMPLETED;
        case 2:
            return TripStatus::CANCELLED;
        default:
            return TripStatus::ONGOING;
    }
}

int TripDAO::createTrip(int driverId, int riderId, int rideRequestId) {
    DBConnection* connection = dbManager->getConnection();
    std::lock_guard<std::mutex> lock(connection->getMutex());
    
    std::stringstream sql;
    sql << "INSERT INTO trips (driver_id, rider_id, ride_request_id, match_time) VALUES ("
        << driverId << ", " << riderId << ", " << rideRequestId << ", \""
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

std::unique_ptr<Trip> TripDAO::getTripById(int id) {
    DBConnection* connection = dbManager->getConnection();
    std::unique_ptr<Trip> trip;
    
    std::stringstream sql;
    sql << "SELECT id, driver_id, rider_id, ride_request_id, match_time, status FROM trips WHERE id = " << id;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return nullptr;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int tripId = sqlite3_column_int(stmt, 0);
        int driverId = sqlite3_column_int(stmt, 1);
        int riderId = sqlite3_column_int(stmt, 2);
        int rideRequestId = sqlite3_column_int(stmt, 3);
        
        trip = std::make_unique<Trip>(tripId, driverId, riderId, rideRequestId);
        trip->setStatus(intToTripStatus(sqlite3_column_int(stmt, 5)));
    }
    
    sqlite3_finalize(stmt);
    return trip;
}

bool TripDAO::updateTrip(const Trip& trip) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE trips SET driver_id = " << trip.getDriverId()
        << ", rider_id = " << trip.getRiderId()
        << ", ride_request_id = " << trip.getRideRequestId()
        << ", status = " << tripStatusToInt(trip.getStatus())
        << " WHERE id = " << trip.getId();
    
    return connection->execute(sql.str());
}

bool TripDAO::updateTripStatus(int id, TripStatus status) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE trips SET status = " << tripStatusToInt(status) << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

std::vector<std::unique_ptr<Trip>> TripDAO::getTripsByDriverId(int driverId) {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Trip>> tripList;
    
    std::stringstream sql;
    sql << "SELECT id, driver_id, rider_id, ride_request_id, match_time, status FROM trips WHERE driver_id = " << driverId;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return tripList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tripId = sqlite3_column_int(stmt, 0);
        int dId = sqlite3_column_int(stmt, 1);
        int riderId = sqlite3_column_int(stmt, 2);
        int rideRequestId = sqlite3_column_int(stmt, 3);
        TripStatus status = intToTripStatus(sqlite3_column_int(stmt, 5));
        
        auto trip = std::make_unique<Trip>(tripId, dId, riderId, rideRequestId);
        trip->setStatus(status);
        
        tripList.push_back(std::move(trip));
    }
    
    sqlite3_finalize(stmt);
    return tripList;
}

std::vector<std::unique_ptr<Trip>> TripDAO::getTripsByRiderId(int riderId) {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Trip>> tripList;
    
    std::stringstream sql;
    sql << "SELECT id, driver_id, rider_id, ride_request_id, match_time, status FROM trips WHERE rider_id = " << riderId;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return tripList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tripId = sqlite3_column_int(stmt, 0);
        int driverId = sqlite3_column_int(stmt, 1);
        int rId = sqlite3_column_int(stmt, 2);
        int rideRequestId = sqlite3_column_int(stmt, 3);
        TripStatus status = intToTripStatus(sqlite3_column_int(stmt, 5));
        
        auto trip = std::make_unique<Trip>(tripId, driverId, rId, rideRequestId);
        trip->setStatus(status);
        
        tripList.push_back(std::move(trip));
    }
    
    sqlite3_finalize(stmt);
    return tripList;
}

std::vector<std::unique_ptr<Trip>> TripDAO::getActiveTrips() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Trip>> tripList;
    
    std::stringstream sql;
    sql << "SELECT id, driver_id, rider_id, ride_request_id, match_time, status FROM trips WHERE status = " 
        << tripStatusToInt(TripStatus::ONGOING);
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return tripList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tripId = sqlite3_column_int(stmt, 0);
        int driverId = sqlite3_column_int(stmt, 1);
        int riderId = sqlite3_column_int(stmt, 2);
        int rideRequestId = sqlite3_column_int(stmt, 3);
        
        auto trip = std::make_unique<Trip>(tripId, driverId, riderId, rideRequestId);
        trip->setStatus(TripStatus::ONGOING);
        
        tripList.push_back(std::move(trip));
    }
    
    sqlite3_finalize(stmt);
    return tripList;
}

std::unique_ptr<Trip> TripDAO::getTripByRideRequestId(int rideRequestId) {
    DBConnection* connection = dbManager->getConnection();
    std::unique_ptr<Trip> trip;
    
    std::stringstream sql;
    sql << "SELECT id, driver_id, rider_id, ride_request_id, match_time, status FROM trips WHERE ride_request_id = " << rideRequestId;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return nullptr;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int tripId = sqlite3_column_int(stmt, 0);
        int driverId = sqlite3_column_int(stmt, 1);
        int riderId = sqlite3_column_int(stmt, 2);
        int rRequestId = sqlite3_column_int(stmt, 3);
        TripStatus status = intToTripStatus(sqlite3_column_int(stmt, 5));
        
        trip = std::make_unique<Trip>(tripId, driverId, riderId, rRequestId);
        trip->setStatus(status);
    }
    
    sqlite3_finalize(stmt);
    return trip;
}

std::vector<std::unique_ptr<Trip>> TripDAO::getAllTrips() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Trip>> tripList;
    
    std::stringstream sql;
    sql << "SELECT id, driver_id, rider_id, ride_request_id, match_time, status FROM trips";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return tripList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tripId = sqlite3_column_int(stmt, 0);
        int driverId = sqlite3_column_int(stmt, 1);
        int riderId = sqlite3_column_int(stmt, 2);
        int rideRequestId = sqlite3_column_int(stmt, 3);
        TripStatus status = intToTripStatus(sqlite3_column_int(stmt, 5));
        
        auto trip = std::make_unique<Trip>(tripId, driverId, riderId, rideRequestId);
        trip->setStatus(status);
        
        tripList.push_back(std::move(trip));
    }
    
    sqlite3_finalize(stmt);
    return tripList;
}

bool TripDAO::deleteTrip(int id) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "DELETE FROM trips WHERE id = " << id;
    
    return connection->execute(sql.str());
}
