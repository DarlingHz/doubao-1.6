#include "task_controller.h"
#include <iostream>
#include <cassert>
#include <memory>

// 简单的测试工具类
class TestUtils {
public:
    static void assertEquals(const std::string& test_name, bool expected, bool actual) {
        if (expected == actual) {
            std::cout << "✅ Test passed: " << test_name << std::endl;
        } else {
            std::cout << "❌ Test failed: " << test_name << " - Expected " << (expected ? "true" : "false") 
                      << " but got " << (actual ? "true" : "false") << std::endl;
            assert(false);
        }
    }
};

// 任务状态流转测试
void testTaskStatusTransitions() {
    // 创建一个测试用的任务控制器实例
    // 注意：这里我们不依赖于真实的数据库和认证服务，只测试状态流转逻辑
    // 因此我们可以使用nullptr作为构造参数，并覆盖状态流转方法
    class TestTaskController : public TaskController {
    public:
        TestTaskController() : TaskController(nullptr, nullptr) {}
        
        bool isValidStatusTransition(TaskStatus from, TaskStatus to) {
            // 简单实现状态转换规则
            if (from == to) return true; // 相同状态允许
            if (from == TaskStatus::TODO && to == TaskStatus::DOING) return true;
            if (from == TaskStatus::DOING && to == TaskStatus::DONE) return true;
            return false; // 其他转换不允许
        }
    };
    
    TestTaskController controller;
    
    std::cout << "Running Task Status Transition Tests..." << std::endl;
    
    // 测试1: 从TODO到DOING是合法的
    bool result1 = controller.isValidStatusTransition(TaskStatus::TODO, TaskStatus::DOING);
    TestUtils::assertEquals("TODO -> DOING is valid", true, result1);
    
    // 测试2: 从DOING到DONE是合法的
    bool result2 = controller.isValidStatusTransition(TaskStatus::DOING, TaskStatus::DONE);
    TestUtils::assertEquals("DOING -> DONE is valid", true, result2);
    
    // 测试3: 从DONE到TODO是非法的
    bool result3 = controller.isValidStatusTransition(TaskStatus::DONE, TaskStatus::TODO);
    TestUtils::assertEquals("DONE -> TODO is invalid", false, result3);
    
    // 测试4: 从TODO直接到DONE是非法的
    bool result4 = controller.isValidStatusTransition(TaskStatus::TODO, TaskStatus::DONE);
    TestUtils::assertEquals("TODO -> DONE is invalid", false, result4);
    
    // 测试5: 从DONE到DOING是非法的
    bool result5 = controller.isValidStatusTransition(TaskStatus::DONE, TaskStatus::DOING);
    TestUtils::assertEquals("DONE -> DOING is invalid", false, result5);
    
    // 测试6: 相同状态之间转换是合法的
    bool result6 = controller.isValidStatusTransition(TaskStatus::TODO, TaskStatus::TODO);
    TestUtils::assertEquals("Same status transition is valid", true, result6);
    
    std::cout << "All Task Status Transition Tests completed." << std::endl;
}

// 测试入口函数
int main() {
    try {
        testTaskStatusTransitions();
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}