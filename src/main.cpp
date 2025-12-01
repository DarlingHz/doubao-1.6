#include "App.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        // 创建应用实例
        App app;
        
        // 初始化应用
        std::cout << "Initializing application..." << std::endl;
        app.initialize();
        
        // 启动服务器
        int port = 8080;
        if (argc > 1) {
            port = std::stoi(argv[1]);
        }
        app.run(port);
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}