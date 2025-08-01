#pragma once

#include "core/common.h"
#include <string>

namespace Platform {

    // 进程实例检查类，用于替代s::Win::PrevInstance
    class PrevInstance {
    public:
        // 构造函数，创建具有指定名称的互斥量
        PrevInstance(const std::wstring& mutexName);
        
        // 析构函数，释放互斥量
        ~PrevInstance();
        
        // 禁止复制
        PrevInstance(const PrevInstance&) = delete;
        PrevInstance& operator=(const PrevInstance&) = delete;
        
        // 检查是否已有实例在运行
        // 返回值：
        // true - 确定有实例在运行
        // false - 确定没有实例在运行
        // 其他情况 - 无法确定，需要用户确认
        bool isRunning() const;
        
    private:
        std::wstring m_mutexName;
        HANDLE m_mutex;
    };

} // namespace Platform