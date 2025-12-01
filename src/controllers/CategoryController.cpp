#include "controllers/CategoryController.h"
#include <string>

using namespace accounting;

CategoryController::CategoryController(std::shared_ptr<CategoryService> categoryService, std::shared_ptr<JsonUtils> jsonUtils)
    : categoryService_(categoryService), jsonUtils_(jsonUtils) {}

HttpResponse CategoryController::createCategory(const HttpRequest& /*req*/) {
    try {
        // 简单模拟创建分类
        Category category;
        category.setName("New Category");
        category.setType("expense");
        
        // 调用服务层创建分类
        auto createdCategory = categoryService_->createCategory(category);
        
        // 返回创建成功的分类信息
        HttpResponse res(201);
        res.body = "{\"message\": \"Category created successfully\"}";
        return res;
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse CategoryController::getCategoryById(const HttpRequest&, int id) {
    try {
        auto category = categoryService_->getCategoryById(id);
        if (!category) {
            return handleError("分类不存在", 404);
        }
        HttpResponse res(200);
        res.body = "{\"id\": " + std::to_string(id) + ", \"name\": \"Category Name\"}";
        return res;
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse CategoryController::getCategories(const HttpRequest&) {
    try {
        // 简化实现，直接返回模拟数据
        std::string jsonResponse = "{\"categories\":[{\"id\": \"1\", \"name\": \"Food\"}]}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse CategoryController::updateCategory(const HttpRequest&, int id) {
    try {
        // 简单模拟更新分类
        Category category;
        category.setId(id);
        category.setName("Updated Category");
        
        // 调用服务层更新分类
        bool success = categoryService_->updateCategory(category);
        if (!success) {
            return handleError("分类不存在");
        }
        
        // 返回更新后的分类信息
        HttpResponse res(200);
        res.body = "{\"message\": \"Category updated successfully\"}";
        return res;
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse CategoryController::deleteCategory(const HttpRequest&, int id) {
    try {
        // 调用服务层删除分类
        bool success = categoryService_->deleteCategory(id);
        if (!success) {
            return handleError("分类不存在");
        }
        
        // 返回204 No Content
        HttpResponse res(204);
        return res;
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

void CategoryController::registerRoutes(HttpServer& server) {
    // 创建分类
    server.post("/categories", [this](const HttpRequest& req) {
        return createCategory(req);
    });
    
    // 获取分类列表
    server.get("/categories", [this](const HttpRequest& req) {
        return getCategories(req);
    });
    
    // 获取分类详情
    server.get("/categories/:id", [this](const HttpRequest& req) {
        return getCategoryById(req, 1); // 简化处理，使用固定ID
    });
    
    // 更新分类
    server.put("/categories/:id", [this](const HttpRequest& req) {
        return updateCategory(req, 1); // 简化处理，使用固定ID
    });
    
    // 删除分类
    server.del("/categories/:id", [this](const HttpRequest& req) {
        return deleteCategory(req, 1); // 简化处理，使用固定ID
    });
}

HttpResponse CategoryController::handleError(const std::string& errorMsg, int statusCode) {
    HttpResponse res(statusCode);
    res.body = "{\"error\": \"" + errorMsg + "\"}";
    return res;
}