// 匹配控制器实现
#include "matching_controller.h"
#include "../service/matching_service.h"
#include "../service/user_service.h"
#include "../utils/logger.h"
#include "../model/entities.h"
#include <sstream>

MatchingController::MatchingController() {
    // 获取服务单例
    matchingService = MatchingService::getInstance();
    userService = UserService::getInstance();
}

MatchingController::~MatchingController() {
    // 不需要释放服务单例
}

void MatchingController::initialize() {
    // 注册出行请求相关路由
    registerRoute("POST", "/travel-requests", &MatchingController::handleCreateTravelRequest);
    registerRoute("GET", "/travel-requests/:id", &MatchingController::handleGetTravelRequest);
    registerRoute("PUT", "/travel-requests/:id", &MatchingController::handleUpdateTravelRequest);
    registerRoute("PUT", "/travel-requests/:id/cancel", &MatchingController::handleCancelTravelRequest);
    registerRoute("GET", "/riders/:riderId/travel-requests", &MatchingController::handleGetTravelRequestsByRider);
    
    // 注册匹配相关路由
    registerRoute("POST", "/matching/trigger", &MatchingController::handleTriggerMatching);
    registerRoute("GET", "/matching/:requestId/result", &MatchingController::handleGetMatchingResult);
    registerRoute("POST", "/matching/:matchingId/accept", &MatchingController::handleAcceptMatching);
    registerRoute("POST", "/matching/:matchingId/reject", &MatchingController::handleRejectMatching);
    
    // 注册行程相关路由
    registerRoute("POST", "/trips", &MatchingController::handleCreateTrip);
    registerRoute("GET", "/trips/:id", &MatchingController::handleGetTrip);
    registerRoute("PUT", "/trips/:id/status", &MatchingController::handleUpdateTripStatus);
    registerRoute("GET", "/riders/:riderId/trips", &MatchingController::handleGetTripsByRider);
    registerRoute("GET", "/drivers/:driverId/trips", &MatchingController::handleGetTripsByDriver);
    registerRoute("PUT", "/trips/:id/complete", &MatchingController::handleCompleteTrip);
}

// 出行请求相关路由处理函数
HttpResponse MatchingController::handleCreateTravelRequest(const HttpRequest& request) {
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    // 验证请求参数
    if (!validateTravelRequest(data)) {
        return createErrorResponse(400, "Invalid travel request data");
    }
    
    try {
        // 创建出行请求
        TravelRequest* travelRequest = userService->createTravelRequest(
            data["rider_id"],
            std::stof(data["start_latitude"]),
            std::stof(data["start_longitude"]),
            std::stof(data["end_latitude"]),
            std::stof(data["end_longitude"]),
            data["start_address"],
            data["end_address"],
            std::stol(data["request_time"])
        );
        
        if (travelRequest) {
            // 返回创建成功的出行请求信息
            std::string jsonResponse = travelRequestToJson(travelRequest);
            delete travelRequest; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(400, "Failed to create travel request");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error creating travel request: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to create travel request: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleGetTravelRequest(const HttpRequest& request) {
    // 获取路径参数
    std::string requestId = getParam(request, "id");
    
    try {
        // 获取出行请求信息
        TravelRequest* travelRequest = userService->getTravelRequestById(requestId);
        
        if (travelRequest) {
            // 返回出行请求信息
            std::string jsonResponse = travelRequestToJson(travelRequest);
            delete travelRequest; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Travel request not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting travel request: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get travel request: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleUpdateTravelRequest(const HttpRequest& request) {
    // 获取路径参数
    std::string requestId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    try {
        // 更新出行请求
        TravelRequest* travelRequest = userService->updateTravelRequest(requestId, data);
        
        if (travelRequest) {
            // 返回更新后的出行请求信息
            std::string jsonResponse = travelRequestToJson(travelRequest);
            delete travelRequest; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Travel request not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating travel request: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update travel request: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleCancelTravelRequest(const HttpRequest& request) {
    // 获取路径参数
    std::string requestId = getParam(request, "id");
    
    try {
        // 取消出行请求
        bool success = userService->cancelTravelRequest(requestId);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Travel request cancelled successfully\"}");
        } else {
            return createErrorResponse(404, "Travel request not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error cancelling travel request: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to cancel travel request: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleGetTravelRequestsByRider(const HttpRequest& request) {
    // 获取路径参数
    std::string riderId = getParam(request, "riderId");
    
    try {
        // 获取乘客的出行请求列表
        std::vector<TravelRequest*> requests = userService->getTravelRequestsByRider(riderId);
        
        // 构建JSON响应
        std::stringstream ss;
        ss << "[";
        
        bool first = true;
        for (TravelRequest* req : requests) {
            if (!first) {
                ss << ",";
            }
            first = false;
            ss << travelRequestToJson(req);
            delete req; // 释放临时对象
        }
        
        ss << "]";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting travel requests: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get travel requests: " + std::string(e.what()));
    }
}

// 匹配相关路由处理函数
HttpResponse MatchingController::handleTriggerMatching(const HttpRequest& request) {
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("request_id") == data.end()) {
        return createErrorResponse(400, "Request ID is required");
    }
    
    try {
        std::string requestId = data["request_id"];
        
        // 触发匹配
        bool success = matchingService->triggerMatching(requestId);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Matching triggered successfully\"}");
        } else {
            return createErrorResponse(404, "Travel request not found or invalid");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error triggering matching: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to trigger matching: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleGetMatchingResult(const HttpRequest& request) {
    // 获取路径参数
    std::string requestId = getParam(request, "requestId");
    
    try {
        // 获取匹配结果
        MatchingResult* result = matchingService->getMatchingResult(requestId);
        
        if (result) {
            // 返回匹配结果
            std::string jsonResponse = matchingResultToJson(result);
            delete result; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Matching result not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting matching result: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get matching result: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleAcceptMatching(const HttpRequest& request) {
    // 获取路径参数
    std::string matchingId = getParam(request, "matchingId");
    
    try {
        // 接受匹配
        bool success = matchingService->acceptMatching(matchingId);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Matching accepted successfully\"}");
        } else {
            return createErrorResponse(404, "Matching not found or expired");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error accepting matching: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to accept matching: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleRejectMatching(const HttpRequest& request) {
    // 获取路径参数
    std::string matchingId = getParam(request, "matchingId");
    
    try {
        // 拒绝匹配
        bool success = matchingService->rejectMatching(matchingId);
        
        if (success) {
            return createSuccessResponse("{\"message\":\"Matching rejected successfully\"}");
        } else {
            return createErrorResponse(404, "Matching not found or expired");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error rejecting matching: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to reject matching: " + std::string(e.what()));
    }
}

// 行程相关路由处理函数
HttpResponse MatchingController::handleCreateTrip(const HttpRequest& request) {
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    // 验证请求参数
    if (!validateTripRequest(data)) {
        return createErrorResponse(400, "Invalid trip data");
    }
    
    try {
        // 创建行程
        Trip* trip = userService->createTrip(
            data["travel_request_id"],
            data["driver_id"]
        );
        
        if (trip) {
            // 返回创建成功的行程信息
            std::string jsonResponse = tripToJson(trip);
            delete trip; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(400, "Failed to create trip");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error creating trip: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to create trip: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleGetTrip(const HttpRequest& request) {
    // 获取路径参数
    std::string tripId = getParam(request, "id");
    
    try {
        // 获取行程信息
        Trip* trip = userService->getTripById(tripId);
        
        if (trip) {
            // 返回行程信息
            std::string jsonResponse = tripToJson(trip);
            delete trip; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Trip not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting trip: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get trip: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleUpdateTripStatus(const HttpRequest& request) {
    // 获取路径参数
    std::string tripId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("status") == data.end()) {
        return createErrorResponse(400, "Status is required");
    }
    
    try {
        // 更新行程状态
        Trip* trip = userService->updateTripStatus(tripId, data["status"]);
        
        if (trip) {
            // 返回更新后的行程信息
            std::string jsonResponse = tripToJson(trip);
            delete trip; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Trip not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error updating trip status: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to update trip status: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleGetTripsByRider(const HttpRequest& request) {
    // 获取路径参数
    std::string riderId = getParam(request, "riderId");
    
    try {
        // 获取乘客的行程列表
        std::vector<Trip*> trips = userService->getTripsByRider(riderId);
        
        // 构建JSON响应
        std::stringstream ss;
        ss << "[";
        
        bool first = true;
        for (Trip* trip : trips) {
            if (!first) {
                ss << ",";
            }
            first = false;
            ss << tripToJson(trip);
            delete trip; // 释放临时对象
        }
        
        ss << "]";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting rider's trips: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get rider's trips: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleGetTripsByDriver(const HttpRequest& request) {
    // 获取路径参数
    std::string driverId = getParam(request, "driverId");
    
    try {
        // 获取车主的行程列表
        std::vector<Trip*> trips = userService->getTripsByDriver(driverId);
        
        // 构建JSON响应
        std::stringstream ss;
        ss << "[";
        
        bool first = true;
        for (Trip* trip : trips) {
            if (!first) {
                ss << ",";
            }
            first = false;
            ss << tripToJson(trip);
            delete trip; // 释放临时对象
        }
        
        ss << "]";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting driver's trips: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get driver's trips: " + std::string(e.what()));
    }
}

HttpResponse MatchingController::handleCompleteTrip(const HttpRequest& request) {
    // 获取路径参数
    std::string tripId = getParam(request, "id");
    
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    try {
        // 获取评分参数（可选）
        float riderRating = data.find("rider_rating") != data.end() ? std::stof(data["rider_rating"]) : 0.0f;
        float driverRating = data.find("driver_rating") != data.end() ? std::stof(data["driver_rating"]) : 0.0f;
        float actualFare = data.find("actual_fare") != data.end() ? std::stof(data["actual_fare"]) : 0.0f;
        
        // 完成行程
        Trip* trip = userService->completeTrip(tripId, riderRating, driverRating, actualFare);
        
        if (trip) {
            // 返回更新后的行程信息
            std::string jsonResponse = tripToJson(trip);
            delete trip; // 释放临时对象
            return createSuccessResponse(jsonResponse);
        } else {
            return createErrorResponse(404, "Trip not found");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error completing trip: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to complete trip: " + std::string(e.what()));
    }
}

// 辅助方法
bool MatchingController::validateTravelRequest(const std::unordered_map<std::string, std::string>& data) {
    // 检查必需字段
    std::vector<std::string> requiredFields = {
        "rider_id", "start_latitude", "start_longitude", "end_latitude", "end_longitude",
        "start_address", "end_address", "request_time"
    };
    
    for (const auto& field : requiredFields) {
        if (data.find(field) == data.end() || data.at(field).empty()) {
            return false;
        }
    }
    
    return true;
}

bool MatchingController::validateTripRequest(const std::unordered_map<std::string, std::string>& data) {
    // 检查必需字段
    if (data.find("travel_request_id") == data.end() || data.at("travel_request_id").empty()) {
        return false;
    }
    
    if (data.find("driver_id") == data.end() || data.at("driver_id").empty()) {
        return false;
    }
    
    return true;
}

std::string MatchingController::travelRequestToJson(const TravelRequest* request) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\":\"" << request->id << "\",";
    ss << "\"rider_id\":\"" << request->riderId << "\",";
    ss << "\"start_latitude\":" << request->startLatitude << ",";
    ss << "\"start_longitude\":" << request->startLongitude << ",";
    ss << "\"end_latitude\":" << request->endLatitude << ",";
    ss << "\"end_longitude\":" << request->endLongitude << ",";
    ss << "\"start_address\":\"" << request->startAddress << "\",";
    ss << "\"end_address\":\"" << request->endAddress << "\",";
    ss << "\"request_time\":" << request->requestTime << ",";
    ss << "\"status\":\"" << request->status << "\",";
    ss << "\"estimated_fare\":" << request->estimatedFare;
    ss << "}";
    return ss.str();
}

std::string MatchingController::matchingResultToJson(const MatchingResult* result) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\":\"" << result->id << "\",";
    ss << "\"request_id\":\"" << result->requestId << "\",";
    ss << "\"driver_id\":\"" << result->driverId << "\",";
    ss << "\"matching_time\":" << result->matchingTime << ",";
    ss << "\"estimated_distance\":" << result->estimatedDistance << ",";
    ss << "\"estimated_time\":" << result->estimatedTime << ",";
    ss << "\"estimated_fare\":" << result->estimatedFare << ",";
    ss << "\"status\":\"" << result->status << "\"";
    ss << "}";
    return ss.str();
}

std::string MatchingController::tripToJson(const Trip* trip) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\":\"" << trip->id << "\",";
    ss << "\"travel_request_id\":\"" << trip->travelRequestId << "\",";
    ss << "\"rider_id\":\"" << trip->riderId << "\",";
    ss << "\"driver_id\":\"" << trip->driverId << "\",";
    ss << "\"start_time\":" << trip->startTime << ",";
    ss << "\"end_time\":" << trip->endTime << ",";
    ss << "\"status\":\"" << trip->status << ",";
    ss << "\"estimated_fare\":" << trip->estimatedFare << ",";
    ss << "\"actual_fare\":" << trip->actualFare << ",";
    ss << "\"rider_rating\":" << trip->riderRating << ",";
    ss << "\"driver_rating\":" << trip->driverRating;
    ss << "}";
    return ss.str();
}
