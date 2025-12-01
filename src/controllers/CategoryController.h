#ifndef CATEGORY_CONTROLLER_H
#define CATEGORY_CONTROLLER_H

#include <string>
#include <memory>
#include "utils/HttpServer.h"
#include "services/CategoryService.h"
#include "utils/JsonUtils.h"

namespace accounting {

class CategoryController {
public:
    CategoryController(std::shared_ptr<CategoryService> categoryService, std::shared_ptr<JsonUtils> jsonUtils);
    
    // 创建分类
    HttpResponse createCategory(const HttpRequest& req);
    
    // 获取分类详情
    HttpResponse getCategoryById(const HttpRequest& req, int id);
    
    // 获取分类列表
    HttpResponse getCategories(const HttpRequest& req);
    
    // 更新分类
    HttpResponse updateCategory(const HttpRequest& req, int id);
    
    // 删除分类
    HttpResponse deleteCategory(const HttpRequest& req, int id);
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    std::shared_ptr<CategoryService> categoryService_;
    std::shared_ptr<JsonUtils> jsonUtils_;
    
    // 处理错误响应
    HttpResponse handleError(const std::string& errorMsg, int statusCode = 400);
};

} // namespace accounting

#endif // CATEGORY_CONTROLLER_H