// 数据库连接实现
#include "db_connection.h"
#include "../config/config.h"
#include <iostream>

// 静态实例初始化
DBManager* DBManager::instance = nullptr;

DBConnection::DBConnection() : db(nullptr) {
}

DBConnection::~DBConnection() {
    close();
}

bool DBConnection::open(const std::string& dbPath) {
    if (db != nullptr) {
        std::cerr << "Database already connected" << std::endl;
        return false;
    }
    
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    
    std::cout << "Database connected successfully" << std::endl;
    return true;
}

void DBConnection::close() {
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
        std::cout << "Database disconnected" << std::endl;
    }
}

bool DBConnection::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (db == nullptr) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool DBConnection::executeQuery(const std::string& sql, int (*callback)(void*, int, char**, char**), void* data) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (db == nullptr) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), callback, data, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool DBConnection::initializeTables() {
    // 创建riders表
    std::string createRidersTable = R"(
    CREATE TABLE IF NOT EXISTS riders (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        phone_number TEXT,
        rating REAL DEFAULT 5.0,
        registration_time TEXT NOT NULL
    );
    )";
    
    // 创建drivers表
    std::string createDriversTable = R"(
    CREATE TABLE IF NOT EXISTS drivers (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        license_plate TEXT NOT NULL UNIQUE,
        car_model TEXT NOT NULL,
        capacity INTEGER NOT NULL,
        status INTEGER DEFAULT 0,  -- 0:OFFLINE, 1:AVAILABLE, 2:ON_TRIP
        rating REAL DEFAULT 5.0,
        current_x INTEGER DEFAULT 0,
        current_y INTEGER DEFAULT 0,
        registration_time TEXT NOT NULL,
        active_trips_count INTEGER DEFAULT 0,
        INDEX idx_driver_status ON drivers(status),
        INDEX idx_driver_location ON drivers(current_x, current_y)
    );
    )";
    
    // 创建ride_requests表
    std::string createRideRequestsTable = R"(
    CREATE TABLE IF NOT EXISTS ride_requests (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        rider_id INTEGER NOT NULL,
        start_x INTEGER NOT NULL,
        start_y INTEGER NOT NULL,
        end_x INTEGER NOT NULL,
        end_y INTEGER NOT NULL,
        earliest_departure_time TEXT NOT NULL,
        latest_departure_time TEXT NOT NULL,
        status INTEGER DEFAULT 0,  -- 0:PENDING, 1:MATCHED, 2:CANCELLED, 3:COMPLETED
        creation_time TEXT NOT NULL,
        FOREIGN KEY (rider_id) REFERENCES riders(id),
        INDEX idx_ride_request_status ON ride_requests(status),
        INDEX idx_ride_request_rider ON ride_requests(rider_id),
        INDEX idx_ride_request_start_location ON ride_requests(start_x, start_y)
    );
    )";
    
    // 创建trips表
    std::string createTripsTable = R"(
    CREATE TABLE IF NOT EXISTS trips (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        driver_id INTEGER NOT NULL,
        rider_id INTEGER NOT NULL,
        ride_request_id INTEGER NOT NULL,
        match_time TEXT NOT NULL,
        status INTEGER DEFAULT 0,  -- 0:ONGOING, 1:COMPLETED, 2:CANCELLED
        FOREIGN KEY (driver_id) REFERENCES drivers(id),
        FOREIGN KEY (rider_id) REFERENCES riders(id),
        FOREIGN KEY (ride_request_id) REFERENCES ride_requests(id),
        INDEX idx_trip_driver ON trips(driver_id),
        INDEX idx_trip_rider ON trips(rider_id),
        INDEX idx_trip_status ON trips(status)
    );
    )";
    
    // 执行创建表的SQL语句
    return execute(createRidersTable) && 
           execute(createDriversTable) && 
           execute(createRideRequestsTable) && 
           execute(createTripsTable);
}

DBManager::DBManager() {
    connection = std::make_unique<DBConnection>();
}

DBManager* DBManager::getInstance() {
    if (instance == nullptr) {
        instance = new DBManager();
    }
    return instance;
}

DBConnection* DBManager::getConnection() {
    return connection.get();
}

bool DBManager::initialize() {
    Config* config = Config::getInstance();
    if (!connection->open(config->getDbPath())) {
        return false;
    }
    
    return connection->initializeTables();
}
