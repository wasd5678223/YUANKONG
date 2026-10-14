#pragma once
#include "pch.h"
#include <atomic>
#include <mutex>
#include <vector>
#include <Windows.h>

class ThreadFuncBase
{};//线程函数基类
typedef int (ThreadFuncBase::* FUNCTYPE)();//成员函数指针类型

// 线程工作对象
class ThreadWorker
{
public:
    ThreadWorker() : thiz(NULL), func(NULL)
    {}

    ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) : thiz(obj), func(f)
    {}

    ThreadWorker(const ThreadWorker& worker) = default;
    ThreadWorker& operator=(const ThreadWorker& worker) = default;
	//这样只需要在调用处使用worker()即可调用成员函数
    int operator()()
    {
        if (IsValid()) {
            return (thiz->*func)();
        }
        return -1;
    }

    bool IsValid()
    {
        return (thiz != NULL) && (func != NULL);
    }
private:
	ThreadFuncBase* thiz;  // 成员函数所属对象指针（待填入的工作函数所在的类对象）
	FUNCTYPE func;  // 成员函数指针(待填入的工作函数)
};

// 线程类(单个)
class EdoyunThread
{
public:
    EdoyunThread()
    {
        m_hThread = NULL;
    }

    ~EdoyunThread()
    {
        Stop();
	}

    bool Start()
    {
        m_bStatus = true;
        m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
        if (!IsValid()) {
            m_bStatus = false;
        }
        return m_bStatus;
    }

	// 判断线程是否有效
    bool IsValid()
    { // 返回true表示有效 返回false表示线程异常或者已经终止
        if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
        return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
    }
    bool Stop()
    {
        if (m_bStatus == false) return true;
        m_bStatus = false;
        return WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
    }

	//把新的工作函数对象更新到线程中
    void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker())
    {
		m_worker.store(worker);// 更新线程工作对象
    }

	// 判断线程是否空闲 true表示空闲 false表示忙碌
    bool IsIdle()
    {
		return !m_worker.load().IsValid();// 判断当前线程是否空闲
    }


private:
    void ThreadWorker()
    {
        while (m_bStatus) {
			::ThreadWorker worker = m_worker.load();// 获取当前线程工作对象
			// 如果工作对象有效，则调用该工作函数
            if (worker.IsValid()) 
            {
				// 调用线程工作函数
                int ret = worker();
                if (ret != 0) {
                    CString str;
                    str.Format(_T("thread found warning code %d\r\n"), ret);
                    OutputDebugString(str);
                }
                if (ret < 0) {
                    m_worker.store(::ThreadWorker());
                }
            }
            else {
                Sleep(1);
            }
        }
    }
    static void ThreadEntry(void* arg)
    {
        EdoyunThread* thiz = (EdoyunThread*)arg;
        if (thiz) {
            thiz->ThreadWorker();
        }
        _endthread();
    }

private:
    HANDLE m_hThread;
    bool m_bStatus; // false 表示线程将要关闭 true 表示线程正在运行
	std::atomic<::ThreadWorker> m_worker;// 原子操作的线程工作对象
};

// 线程池类
class EdoyunThreadPool {
public:
    EdoyunThreadPool(size_t size) {
        m_threads.resize(size);
    }

    ~EdoyunThreadPool() {
        Stop();
        m_threads.clear();
    }

    bool Invoke() {
        bool ret = true;
        for (size_t i = 0; i < m_threads.size(); ++i) {
            if (m_threads[i].Start() == false) {
                ret = false;
                break;
            }
        }
        if (ret == false) {
            for (size_t i = 0; i < m_threads.size(); ++i) {
                m_threads[i].Stop();
            }
        }
        return ret;
    }
    void Stop()
    {
        for (size_t i = 0; i < m_threads.size(); ++i) {
            m_threads[i].Stop();
        }
    }

	//返回-1表示分配失败，没有空闲线程，返回线程索引表示分配成功
    int DispatchWorker(const ::ThreadWorker& worker)
    {
        int index = -1;
        m_lock.lock();
        for (size_t i = 0; i < m_threads.size(); i++) {
            if (m_threads[i].IsIdle()) {
                m_threads[i].UpdateWorker(worker);
                index = i;
                break;
            }
        }
        m_lock.unlock();
        return index;
    }

	//检查指定索引的线程是否有效
    bool CheckThreadValid(size_t index)
    {
        if (index < m_threads.size()) 
        {
            return m_threads[index].IsValid();
        }
        return false;
    }

private:
	std::mutex m_lock;
	std::vector<EdoyunThread> m_threads;// 线程池中的线程
};




