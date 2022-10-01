/*
 * @Author: kongxinglong kongxinglong@uniontech.com
 * @Date: 2022-08-28 10:44:51
 * @LastEditors: kongxinglong kongxinglong@uniontech.com
 * @LastEditTime: 2022-08-29 09:38:17
 * @FilePath: /webserver/locker.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */


#ifndef LOCKER_H
#define LOCKER_H
#include <pthread.h>
#include <exception>
#include <semaphore.h>
//线程同步机制封装类 

//互斥锁类
class locker{
public:
//成员方法
    //构造函数
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0) //判断创建一个互斥锁
            throw std::exception();
            //失败抛出异常
    }
    //析构函数
    ~locker(){
        pthread_mutex_destroy(&m_mutex)==0;
    }
    //上锁
    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }
    //解锁
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }

    //获取互斥量成员
    pthread_mutex_t *get(){
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;  //创建一个互斥锁
};


//条件变量类,判断是否有数据
class cond{
public:
    //构造函数
    cond(){
        if(pthread_cond_init(&m_cond,NULL)!=0);
            throw std::exception();
    }

    //析构函数
    ~cond(){
        pthread_cond_destroy(&m_cond);
    }

    //配合互斥锁实现条件变量
    bool wait(pthread_mutex_t *mutex){
        return pthread_cond_wait(&m_cond,mutex)==0;
    }

    //带超时时间
    bool timewait(pthread_mutex_t *mutex,struct timespec t){
        return pthread_cond_timedwait(&m_cond,mutex,&t)==0;
    }
    
    //让条件变量增加
    bool signal(){
        return pthread_cond_signal(&m_cond)==0;
    }
    //将所有线程都唤醒
    bool broadcat(){
        return pthread_cond_broadcast(&m_cond)==0;
    }

private:
    pthread_cond_t m_cond;

};

//信号量相关类
class sem{
public:
    //构造函数
    sem(){
        if(sem_init(&m_sem,0,0)!=0)
            throw std::exception();
    }
    //有参构造函数
    sem(int num){
        if(sem_init(&m_sem,0,num)!=0){
            throw std::exception();
        }
    }
    //析构函数
    ~sem(){
        sem_destroy(&m_sem);
    }

    //等待信号量
    bool wait(){
        return sem_wait(&m_sem)==0;
    }

    //增加信号量
    bool post(){
        return sem_post(&m_sem)==0;
    }

private:
    sem_t m_sem;

};



#endif