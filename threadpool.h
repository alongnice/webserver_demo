/*
 * @Author: kongxinglong kongxinglong@uniontech.com
 * @Date: 2022-08-28 11:39:08
 * @LastEditors: kongxinglong kongxinglong@uniontech.com
 * @LastEditTime: 2022-08-31 10:34:26
 * @FilePath: /webserver/htreadpool.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <list>
#include "locker.h"
#include <exception>
#include <cstdio>

//线程池类,定义成模板类,为了代码复用,模板参数T就是任务类
template<typename T>
class threadpool{
public:
    //成员方法
    //构造函数
    threadpool(int thread_number=8,int max_requests=10000);
    ~threadpool();

    //添加任务的方法
    bool append(T* request);

private:
    static void* worker(void *arg);
    void run();


private:
    //线程的数量
    int m_thread_number;
    //线程池的容器 -> 数组实现大小为 m_thread_number
    pthread_t *m_threads;

    //请求队列中最多允许的,等待处理的请求数量
    int m_max_requests;

    //请求队列,队列中是任务类
    std::list< T*> m_workqueue;
    
    //互斥锁
    locker m_queuelocker;

    //定义信号量,判断是否有任务需要处理
    sem m_queuestat;

    //是否结束线程
    bool m_stop;

};

template<typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):m_thread_number(thread_number),m_max_requests(max_requests),m_stop(false),m_threads(NULL){
    if((thread_number<=0)||(max_requests<=0))
        throw std::exception();
    //创建数组
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads)
        throw std::exception();

    //创建thread_number个线程,并将他们设置为线程脱离
    for(int i=0;i<thread_number;++i){
        printf("正在创建第%d个线程\n",i+1);
        //执行创建
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){
            delete [] m_threads;
            throw std::exception();
        //worker 在c中是全局函数  c++中是static函数
        //NULL表示没有采用参数
        }
        //如果出错抛出异常
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool(){
    delete [] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request){
    m_queuelocker.lock();
    //如果任务队列超出最大
    if(m_workqueue.size()>m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    //否则存入任务队列
    m_workqueue.push_back(request);
    //解锁
    m_queuelocker.unlock();
    //信号量增加
    m_queuestat.post();
    return true;

}

template<typename T>
void * threadpool<T>::worker(void *arg){
//静态成员函数无法访问非静态成员函数
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        //取一个任务
        //通过信号量调用wait,如有值信号量-1,如果没有值(任务)阻塞
        m_queuestat.wait();
        //操作队列,上锁
        m_queuelocker.lock();
        //判断工作队列
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            //解锁
            continue;
        }
        //说明有数据
        T* request = m_workqueue.front();
        //获取第一个任务
        m_workqueue.pop_front();
        //解锁操作
        m_queuelocker.unlock();
        //如果说获取到
        if(!request){
            continue;
        }
        request->process();
        //任务类
    }
}



#endif