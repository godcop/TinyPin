#include "core/stdafx.h"
#include "platform/process_manager.h"

// 进程实例检查类实现

Platform::PrevInstance::PrevInstance(const std::wstring& mutexName) : m_mutexName(mutexName), m_mutex(NULL) {
    // 在构造函数中不立即创建互斥量，而是在isRunning()中进行检查和创建
}

Platform::PrevInstance::~PrevInstance() {
    if (m_mutex) {
        // 释放互斥量的所有权并关闭句柄
        ReleaseMutex(m_mutex);
        CloseHandle(m_mutex);
        m_mutex = NULL;
    }
}

bool Platform::PrevInstance::isRunning() const {
    // 如果还没有创建互斥量，现在创建
    if (!m_mutex) {
        // 尝试创建命名互斥量，如果已存在则会返回现有的句柄
        const_cast<PrevInstance*>(this)->m_mutex = CreateMutexW(NULL, FALSE, m_mutexName.c_str());
        
        if (!m_mutex) {
            // 创建互斥体失败，假设有其他实例在运行（安全起见）
            return true;
        }
        
        DWORD lastError = GetLastError();
        if (lastError == ERROR_ALREADY_EXISTS) {
            // 互斥体已存在，另一个实例正在运行
            return true;
        }
        
        // 互斥体创建成功且没有错误，表示这是第一个实例
        // 现在尝试获取互斥量的所有权
        DWORD waitResult = WaitForSingleObject(m_mutex, 0);
        if (waitResult == WAIT_OBJECT_0) {
            // 成功获取所有权，这是第一个实例
            return false;
        } else {
            // 无法获取所有权，说明有其他实例在运行
            return true;
        }
    }
    
    // 互斥量已经创建，检查是否能获取所有权
    DWORD waitResult = WaitForSingleObject(m_mutex, 0);
    if (waitResult == WAIT_OBJECT_0) {
        // 成功获取所有权，但这不应该发生，因为我们应该已经拥有它
        // 释放所有权以保持一致性
        ReleaseMutex(m_mutex);
        return false;
    } else {
        // 无法获取所有权，说明有其他实例在运行
        return true;
    }
}