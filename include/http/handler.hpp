#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "http/http_request.hpp"
#include "http/http_response.hpp"

namespace api_quota {
namespace http {

// 注意：由于路由功能已经在HttpRouter类中实现，
// 这个文件主要是为了满足CMakeLists.txt中的依赖关系
// 在实际实现中，处理逻辑直接在HttpRouter中完成

// HTTP处理器接口（作为路由的补充）
class HttpRequestHandler {
public:
    virtual ~HttpRequestHandler() = default;
    
    // 处理请求的接口方法
    virtual HttpResponse handle_request(const HttpRequest& request) = 0;
};

} // namespace http
} // namespace api_quota

#endif // HANDLER_HPP