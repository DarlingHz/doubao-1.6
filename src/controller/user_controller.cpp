// 用户控制器实现
#include "user_controller.h"
#include "../service/user_service.h"
#include "../utils/logger.h"
#include "../model/entities.h"
#include <sstream>

UserController::UserController() {
    // 获取用户服务单例
    userService = UserService::getInstance();
}

UserController::~UserController() {
    // 不需要释放userService，它是单例
}

void UserController::initialize() {
    // 注册乘客相关路由
    registerRoute("POST", "/riders", &UserController::handleCreateRider);
    registerRoute("GET", "/riders/:id", &UserController::handleGetRider);
    registerRoute("PUT", "/riders/:id", &UserController::handleUpdateRider);
    registerRoute("DELETE", "/riders/:id", &UserController::handleDeleteRider);
    registerRoute("PUT", "/riders/:id/rating", &UserController::handleUpdateRiderRating);
    
    // 注册车主相关路由
    registerRoute("POST", "/drivers", &UserController::handleCreateDriver);
    registerRoute("GET", "/drivers/:id", &UserController::handleGetDriver);
    registerRoute("PUT", "/drivers/:id", &UserController::handleUpdateDriver);
    registerRoute("PUT", "/drivers/:id/status", &UserController::handleUpdateDriverStatus);
    registerRoute("PUT", "/drivers/:id/location", &UserController::handleUpdateDriverLocation);
    registerRoute("PUT", "/drivers/:id/rating", &UserController::handleUpdateDriverRating);
    registerRoute("DELETE", "/drivers/:id", &UserController::handleDeleteDriver);
    registerRoute("GET", "/drivers/available", &UserController::handleGetAvailableDrivers);
}

// 乘客相关路由处理函数
HttpResponse UserController::handleCreateRider(const HttpRequest& request) {
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    // 验证请求参数
    if (!validateRiderRequest(data)) {
        return createErrorResponse(400, "Invalid rider data");
    }
    
    try {
        // 创建乘客
        Rider* rider = userService->createRider(
            data["phone"],
            data["name"],
            data["gender"],
            std::stoi(data["age"]),
            data["payment_method"]
        );
        
        if (rider) {
            // 返回创建成功的乘客信息
            std::string jsonResponse = riderToJson(rider);
            delete rider; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(400, "Failed to create rider");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error creating rider: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to create rider: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleGetRider(const HttpRequest& request) {
    // 获取路径参数
    std::string riderId = getParam(request, "id");
    
    try {
        // 获取乘客信息
        Rider* rider = userService->getRiderById(riderId);
        
        if (rider) {
            // 返回乘客信息
            std::string jsonResponse = riderToJson(rider);
            delete rider; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Rider not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting rider: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get rider: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleUpdateRider(const HttpRequest& request) {
    // 获取路径参数
    std::string riderId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    try {
        // 更新乘客信息
        Rider* rider = userService->updateRider(riderId, data);
        
        if (rider) {
            // 返回更新后的乘客信息
            std::string jsonResponse = riderToJson(rider);
            delete rider; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Rider not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating rider: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update rider: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleDeleteRider(const HttpRequest& request) {
    // 获取路径参数
    std::string riderId = getParam(request, "id");
    
    try {
        // 删除乘客
        bool success = userService->deleteRider(riderId);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Rider deleted successfully\"}");
        } else {
            return createErrorResponse(404, "Rider not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error deleting rider: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to delete rider: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleUpdateRiderRating(const HttpRequest& request) {
    // 获取路径参数
    std::string riderId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("rating") == data.end()) {
        return createErrorResponse(400, "Rating is required");
    }
    
    try {
        float rating = std::stof(data["rating"]);
        
        // 更新乘客评分
        bool success = userService->updateRiderRating(riderId, rating);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Rating updated successfully\"}");
        } else {
            return createErrorResponse(404, "Rider not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating rider rating: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update rating: " + std::string(e.what()));
    }
}

// 车主相关路由处理函数
HttpResponse UserController::handleCreateDriver(const HttpRequest& request) {
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    // 验证请求参数
    if (!validateDriverRequest(data)) {
        return createErrorResponse(400, "Invalid driver data");
    }
    
    try {
        // 创建车主
        Driver* driver = userService->createDriver(
            data["phone"],
            data["name"],
            data["gender"],
            std::stoi(data["age"]),
            data["car_model"],
            data["license_plate"],
            data["driver_license"],
            std::stof(data["initial_rating"])
        );
        
        if (driver) {
            // 返回创建成功的车主信息
            std::string jsonResponse = driverToJson(driver);
            delete driver; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(400, "Failed to create driver");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error creating driver: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to create driver: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleGetDriver(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "id");
    
    try {
        // 获取车主信息
        Driver* driver = userService->getDriverById(driverId);
        
        if (driver) {
            // 返回车主信息
            std::string jsonResponse = driverToJson(driver);
            delete driver; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Driver not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting driver: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get driver: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleUpdateDriver(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    try {
        // 更新车主信息
        Driver* driver = userService->updateDriver(driverId, data);
        
        if (driver) {
            // 返回更新后的车主信息
            std::string jsonResponse = driverToJson(driver);
            delete driver; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Driver not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating driver: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update driver: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleUpdateDriverStatus(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("status") == data.end()) {
        return createErrorResponse(400, "Status is required");
    }
    
    try {
        // 更新车主状态
        bool success = userService->updateDriverStatus(
            driverId, 
            data["status"] == "available"
        );
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Status updated successfully\"}");
        } else {
            return createErrorResponse(404, "Driver not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating driver status: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update status: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleUpdateDriverLocation(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("latitude") == data.end() || data.find("longitude") == data.end()) {
        return createErrorResponse(400, "Latitude and longitude are required");
    }
    
    try {
        float latitude = std::stof(data["latitude"]);
        float longitude = std::stof(data["longitude"]);
        
        // 更新车主位置
        bool success = userService->updateDriverLocation(
            driverId, 
            latitude, 
            longitude
        );
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Location updated successfully\"}");
        } else {
            return createErrorResponse(404, "Driver not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating driver location: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update location: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleUpdateDriverRating(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("rating") == data.end()) {
        return createErrorResponse(400, "Rating is required");
    }
    
    try {
        float rating = std::stof(data["rating"]);
        
        // 更新车主评分
        bool success = userService->updateDriverRating(driverId, rating);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Rating updated successfully\"}");
        } else {
            return createErrorResponse(404, "Driver not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating driver rating: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update rating: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleDeleteDriver(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "id");
    
    try {
        // 删除车主
        bool success = userService->deleteDriver(driverId);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Driver deleted successfully\"}");
        } else {
            return createErrorResponse(404, "Driver not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error deleting driver: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to delete driver: " + std::string(e.what()));
    }
}

HttpResponse UserController::handleGetAvailableDrivers(const HttpRequest& request) {
    // 获取查询参数
    float latitude = getFloatParam(request, "latitude");
    float longitude = getFloatParam(request, "longitude");
    float radius = getFloatParam(request, "radius", 5.0f); // 默认5公里范围
    
    try {
        // 获取附近可用车主
        std::vector<Driver*> drivers = userService->getAvailableDrivers(latitude, longitude, radius);
        
        // 构建JSON响应
        std::stringstream ss;
        ss << "[";
        
        bool first = true;
        for (Driver* driver : drivers) {
            if (!first) {
                ss << ",";
            }
            first = false;
            ss << driverToJson(driver);
            delete driver; // 释放临时对象
        }
        
        ss << "]";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting available drivers: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get available drivers: " + std::string(e.what()));
    }
}

// 辅助方法
bool UserController::validateRiderRequest(const std::unordered_map<std::string, std::string>& data) {
    // 检查必需字段
    std::vector<std::string> requiredFields = {"phone", "name", "gender", "age", "payment_method"};
    for (const auto& field : requiredFields) {
        if (data.find(field) == data.end() || data.at(field).empty()) {
            return false;
        }
    }
    
    // 验证手机号
    if (!userService->isValidPhoneNumber(data.at("phone"))) {
        return false;
    }
    
    return true;
}

bool UserController::validateDriverRequest(const std::unordered_map<std::string, std::string>& data) {
    // 检查必需字段
    std::vector<std::string> requiredFields = {"phone", "name", "gender", "age", "car_model", 
                                             "license_plate", "driver_license", "initial_rating"};
    for (const auto& field : requiredFields) {
        if (data.find(field) == data.end() || data.at(field).empty()) {
            return false;
        }
    }
    
    // 验证手机号
    if (!userService->isValidPhoneNumber(data.at("phone"))) {
        return false;
    }
    
    // 验证车辆信息
    if (!userService->isValidVehicleInfo(data.at("license_plate"), data.at("driver_license"))) {
        return false;
    }
    
    return true;
}

std::string UserController::riderToJson(const Rider* rider) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\":\"" << rider->id << "\",";
    ss << "\"phone\":\"" << rider->phone << "\",";
    ss << "\"name\":\"" << rider->name << "\",";
    ss << "\"gender\":\"" << rider->gender << "\",";
    ss << "\"age\":" << rider->age << ",";
    ss << "\"rating\":" << rider->rating << ",";
    ss << "\"payment_method\":\"" << rider->paymentMethod << "\"";
    ss << "}";
    return ss.str();
}

std::string UserController::driverToJson(const Driver* driver) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\":\"" << driver->id << "\",";
    ss << "\"phone\":\"" << driver->phone << "\",";
    ss << "\"name\":\"" << driver->name << "\",";
    ss << "\"gender\":\"" << driver->gender << "\",";
    ss << "\"age\":" << driver->age << ",";
    ss << "\"rating\":" << driver->rating << ",";
    ss << "\"car_model\":\"" << driver->carModel << "\",";
    ss << "\"license_plate\":\"" << driver->licensePlate << "\",";
    ss << "\"available\":" << (driver->available ? "true" : "false") << ",";
    ss << "\"latitude\":" << driver->latitude << ",";
    ss << "\"longitude\":" << driver->longitude;
    ss << "}";
    return ss.str();
}
