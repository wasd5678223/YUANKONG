#pragma once
#include <windows.h>
#include <atomic>
#include <list>


template <class T>
class CEdoyunQueue
{//线程安全的队列，基于IOCP实现
public:
    enum
    {
		EQNone,
		EQPush,
		EQPop,
        EQSize,
        EQClear,
    };
    typedef struct IocpParam
    {
        size_t nOperator; // 操作
        T strData; // 数据
        HANDLE hEvent; //pop同步事件
        // 构造函数
        IocpParam(size_t op, const T& data, HANDLE hEve = NULL)
        {
            nOperator = op;
            strData = data;
			hEvent = hEve;

        }

		// 默认构造函数，防止未初始化
        IocpParam()
        {
            nOperator = EQNone;
        }
    }PPARAM;//用于投递信息

    
public:
    CEdoyunQueue()
    {
		m_lock = false;
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//创建完成端口
        m_hThread = INVALID_HANDLE_VALUE;
        if (m_hIOCP != INVALID_HANDLE_VALUE && m_hIOCP != NULL)
        {
			m_hThread = (HANDLE)_beginthread
            (&CEdoyunQueue<T>::threadEntry,
                0, this);//创建工作线程
        }

    }

    ~CEdoyunQueue()
    {
        if(m_lock)
        {
            return;//已经在析构过程中，直接返回
		}
		m_lock = true;
		PostQueuedCompletionStatus(m_hIOCP, 0, NULL, NULL);//通知工作线程退出
		WaitForSingleObject(m_hThread, INFINITE);//等待工作线程退出
		if (m_hIOCP != NULL)
		{

			//防御性编程，避免多次关闭句柄
			HANDLE hIOCP = m_hIOCP;//先保存句柄
			m_hIOCP = NULL;//置空成员变量，防止其他地方继续使用
			CloseHandle(hIOCP);//关闭完成端口句柄
		}
    }
    bool PushBack(const T& data)
    {
		
        // 投递Push事件：
        // 参数1：IOCP句柄
        // 参数2：sizeof(IOCP_PARAM)（dwTransferred，仅占位）
        // 参数3：CompletionKey（new的IocpParam对象，指定Push操作，数据为data）
        // 参数4：NULL（OVERLAPPED，仅占位）
		IocpParam* pParam = new IocpParam(EQPush, data);//构造投递参数结构体
        if (m_lock)
        {
            //队列正在析构，拒绝操作
            delete pParam;
            return false;
        }
		bool ret = PostQueuedCompletionStatus(m_hIOCP, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
        if(ret == false)
        {
			delete pParam;//投递失败，释放内存
        }
        return ret;

    }
    bool PopFront(T& data)
    {
        IocpParam pParam(EQPop, data);//构造投递参数结构体
		pParam.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//创建同步事件
        if (m_lock)
        {
            return false;//队列正在析构，拒绝操作
            CloseHandle(pParam.hEvent);
            return false;

        }
        bool ret = PostQueuedCompletionStatus(m_hIOCP, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
        if (ret == false)
        {
			CloseHandle(pParam.hEvent);
            return false;
        }
		//等待工作线程处理完Pop请求
		ret = (WaitForSingleObject(pParam.hEvent, INFINITE) == WAIT_OBJECT_0);
        if (ret)
        {
			data = pParam.strData;//获取弹出的数据
        }
        CloseHandle(pParam.hEvent);
        return ret;

    }
    size_t Size()
    {
        IocpParam pParam(EQSize, T());//构造投递参数结构体 
        pParam.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//创建同步事件  
        if (m_lock)
        {
            return false;//队列正在析构，拒绝操作
            CloseHandle(pParam.hEvent);
            return -1;
        
        }

        bool ret = PostQueuedCompletionStatus(m_hIOCP, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
        if (ret == false)
        {
            CloseHandle(pParam.hEvent);
            return -1;
        }
        //等待工作线程处理完Pop请求
        ret = (WaitForSingleObject(pParam.hEvent, INFINITE) == WAIT_OBJECT_0);
        if (ret)
        {
			return pParam.nOperator;//Size操作时，nOperator存储队列大小
        }
        CloseHandle(pParam.hEvent);
        return -1;
    }
    bool Clear()
    {
        if (m_lock)return false;//队列正在析构，拒绝操作
        // 投递Push事件：
        // 参数1：IOCP句柄
        // 参数2：sizeof(IOCP_PARAM)（dwTransferred，仅占位）
        // 参数3：CompletionKey（new的IocpParam对象，指定Push操作，数据为data）
        // 参数4：NULL（OVERLAPPED，仅占位）
        IocpParam* pParam = new IocpParam(EQPush, T()); //构造投递参数结构体
        bool ret = PostQueuedCompletionStatus(m_hIOCP, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
        if (ret == false)
        {
            delete pParam; //投递失败，释放内存
            return false; // 返回失败状态
        }
        return ret;


    }
private:
    static void threadEntry(void* arg)
    {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
        thiz->threadMain();
		_endthread();
    }
    void threadMain()
    {
        std::list<std::string> lstString;  // 核心队列（存储字符串）
        DWORD dwTransferred = 0;           // 传输字节数（IOCP参数，这里仅占位）
		PPARAM* pParam = NULL;         // 投递参数结构体指针
        ULONG_PTR CompletionKey = 0;       // 自定义键（指向IocpParam对象）
        OVERLAPPED* pOverlapped = NULL;    // 重叠结构（IOCP异步IO用，这里仅占位）
        //循环监听完成端口事件（ 阻塞等待IOCP事件：INFINITE表示无限等待）
        while (GetQueuedCompletionStatus(
            m_hIOCP,
            &dwTransferred,
            &CompletionKey, 
            &pOverlapped, INFINITE))
        {
            // 退出条件：传输字节数为0 或 CompletionKey为空（主线程投递退出信号）
            if ((dwTransferred == 0) || (CompletionKey == NULL))
            {
                printf("thread is prepare to exit!\r\n");
                break;
            }
            
            pParam = (PPARAM*)CompletionKey;
            DealParam(pParam);
            
        } 
        //防止数据残留(不阻塞直接检查有没有数据)
        while (GetQueuedCompletionStatus(
            m_hIOCP,
            &dwTransferred,
            &CompletionKey,
            &pOverlapped, 0))
        {
            // 退出条件：传输字节数为0 或 CompletionKey为空（主线程投递退出信号）
            if ((dwTransferred == 0) || (CompletionKey == NULL))
            {
                printf("thread is prepare to exit!\r\n");
                continue;
            }
            else
            {
                pParam = (PPARAM*)CompletionKey;
                DealParam(pParam);
            }
        }
    }

    void DealParam(PPARAM* pParam)
    {
        switch (pParam->nOperator)
        {
        case EQPush:
        {
            //处理Push操作
            m_lstData.push_back(pParam->strData);
            delete pParam;
        }
        break;
        case EQPop:
        {
            //处理Pop操作
            if (m_lstData.size() > 0) {
                pParam->strData = m_lstData.front();//获取弹出数据
                m_lstData.pop_front();//弹出队列中顶部数据
            }
            if (pParam->hEvent != NULL) {
                SetEvent(pParam->hEvent);//通知等待线程
            }
        }
        break;
        case EQSize:
        {
            //处理Size操作
            pParam->nOperator = m_lstData.size();
            if (pParam->hEvent != NULL)
                SetEvent(pParam->hEvent);
        }
        break;
        case EQClear:
        {
            //处理Clear操作
            m_lstData.clear();
            delete pParam;
        }
        break;
        default:
            break;
        }
    }

private:
	std::list<T> m_lstData; // 数据列表，支持 push_back、pop_front、clear 等操作
	HANDLE m_hIOCP;         // 完成端口句柄
	HANDLE m_hThread;       // 工作线程句柄
	std::atomic<bool> m_lock;//互斥锁(队列析构)






};

