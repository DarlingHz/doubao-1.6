#ifndef ROUTER_H
#define ROUTER_H

#include <string>
#include <map>
#include <functional>
#include <vector>

namespace http {

struct HttpRequest;
struct HttpResponse;

class Router {
public:
    using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;
    using RouteHandler = std::function<bool(const HttpRequest&, HttpResponse&)>;

    void addRoute(const std::string& method, const std::string& path, Handler handler);
    bool route(const HttpRequest& request, HttpResponse& response);

private:
    struct Route {
        std::string method;
        std::string path;
        Handler handler;
    };

    std::vector<Route> routes;
    std::map<std::string, std::string> extractPathParams(const std::string& path, const std::string& routePath) const;
    bool matchPath(const std::string& path, const std::string& routePath, std::map<std::string, std::string>& params) const;
};

} // namespace http

#endif // ROUTER_H