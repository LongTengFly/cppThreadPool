#ifndef _CPP_THREAD_POOL_H_
#define _CPP_THREAD_POOL_H_

#include <iostream>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <future>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

using namespace std;

void getNow(timeval *tv);
int64_t getNowMs();

#define TNOW    getNow()
#define TNOWMS  getNowMs()

class CPP_ThreadPool{
protected:
    struct TaskFunc{
        TaskFunc(uint64_t expireTime):_expireTime(expireTime){}
        int64_t _expireTime=0;//超时的绝对时间
        function<void()> _func;
    };
    typedef shared_ptr<TaskFunc> TaskFuncPtr;

    /* 
     * @brief 获取任务 ** 
     *@return TaskFuncPtr 
     */
    bool get(TaskFuncPtr& task);

    /*
    * @brief 线程池是否退出
    */
    bool isTerminate()
    {
        return _bTerminate;
    }

    /*
    * @brief 线程运行态
    */
   void run();

public: 
    /*
     * @brief 构造函数 
     */
    CPP_ThreadPool(); 

    /* 
    * @brief 析构, 会停止所有线程 
    */
    virtual ~CPP_ThreadPool();

    /* 
     * * @brief 初始化. 
     * * @param num 工作线程个数 
     */
    bool init(size_t num);

    /*
    * @brief 停止所有线程, 会等待所有线程结束 
    */
   void stop();

   /*
   * @brief 启动所有线程 
   */
    bool start();

    /* 
     * @brief 等待当前任务队列中, 所有工作全部结束(队列无任务). 
     * @param millsecond 等待的时间(ms), -1:永远等待 
     * @return true, 所有工作都处理完毕 
     * false,超时退出 
     */
    bool waitForAllDone(int millsecond=-1);

   /*
    * @brief 获取线程个数.
    * @return size_t 线程个数 
    */
   size_t getThreadNum()
   {
        unique_lock<mutex> lock(_mutex);
        return _threads.size();
   }

   /*
    *  @brief 获取当前线程池的任务数
    * @return size_t 线程池的任务数 
    */
   size_t getJobNum()
   {
        unique_lock<mutex> lock(_mutex);
        return _tasks.size();
   }

   /*
   * @brief 用线程池启用任务(F是function, Args是参数) ** 
   * @param ParentFunctor 
   * @param tf 
   * @return 返回任务的future对象, 可以通过这个对象来获取返回值 
   */
    template <class F,class... Args>
    auto exec(F&& f, Args&&... args)->future<decltype(f(args...))>
    {
        return exec(0,f,args...);
    }

    /* 
    * unused.
    *
    * @brief 用线程池启用任务(F是function, Args是参数) 
    * @param 超时时间 ，单位ms (为0时不做超时控制) ；若任务超时，此任务将被丢弃 
    * @param bind function 
    * @return 返回任务的future对象, 可以通过这个对象来获取返回值 
    *
    * template <class F, class... Args> 
    * 它是c++里新增的最强大的特性之一，它对参数进行了高度泛化，它能表示0到任意个数、任意类型的参数 
    * auto exec(F &&f, Args &&... args) -> std::future<decltype(f(args...))> 
    * std::future<decltype(f(args...))>：返回future，调用者可以通过future获取返回值 
    * 返回值后置
    */
    template<class F,class... Args>
    auto exec(int64_t timeoutMs,F&& f,Args&&... args) -> future<decltype(f(args...))>
    {
        //获取现在时间
        int64_t expireTime=(timeoutMs==0)?0:TNOWMS+timeoutMs;
        // 定义返回值类型
        using retType=decltype(f(args...));
        // 封装任务
        auto task=make_shared<packaged_task<retType()>>(bind(forward<F>(f),forward<Args>(args)...));
        // 封装任务指针，设置过期时间
        TaskFuncPtr fPtr=make_shared<TaskFunc>(expireTime);
        fPtr->_func=[task](){
            (*task)();
        };

        unique_lock<mutex> lock(_mutex);
        // 插入任务
        _tasks.push(fPtr);
        // 唤醒阻塞的线程，可以考虑只有任务队列为空的情 况再去notify
        _condition.notify_one();

        return task->get_future();
    }

protected:
    size_t  _threadNum;//线程数量
    bool    _bTerminate;//判定是否终止线程池
    mutex   _mutex;     //唯一锁
    vector<thread*> _threads;   //工作线程数组
    queue<TaskFuncPtr> _tasks;  //任务队列
    condition_variable _condition;//条件变量
    atomic<int>         _atomic{0};//原子变量
};


#endif