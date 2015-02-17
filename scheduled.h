/*
 * Scheduled
 * Copyright (C) 2014, Xuan <xuanmgr@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Github: https://github.com/Kxuan/scheduled/issues
 * Please tell me the trouble by issue or e-mail.
 * :)
 */
#ifndef __SCHEDULED_H_INCLUDED__
#define __SCHEDULED_H_INCLUDED__

#include <set>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

template<typename ClockType = std::chrono::system_clock>
class Scheduled
{
public:
    class TaskNode;

    typedef void(*callback)(void *what);

    typedef typename std::multiset<std::shared_ptr<TaskNode>>::iterator mset_iter;
    typedef typename std::multiset<std::shared_ptr<TaskNode>>::const_iterator mset_const_iter;

    class TaskNode
    {
        friend class Scheduled;

    public:
        ~TaskNode()
        {
            if (cleanCallback)
                cleanCallback(pData);
        }

        //Cancel the TaskNode
        bool cancel() const
        {
            return parent->remove(this);
        }

        void run()
        {
            try
            {
                taskCallback(pData);
            } catch (...)
            {
                if (throwCallback)
                    throwCallback(pData);
            }
        }

    private:
        TaskNode(Scheduled<ClockType> *parent,
                 std::chrono::time_point<ClockType> const &time,
                 callback cbTask,
                 void *data)
                : parent(parent), time(time), pData(data), taskCallback(cbTask)
        {

        }

        Scheduled<ClockType> *parent;
        std::chrono::time_point<ClockType> time;
        void *pData;

        callback taskCallback, throwCallback, cleanCallback;
    };

    class TaskCompare
    {
    public:
        bool operator()(std::shared_ptr<TaskNode> const &t1, std::shared_ptr<TaskNode> const &t2) const
        {
            return t1->time < t2->time;
        }
    };

    class WorkerController
    {
        friend class Scheduled;

    public:
        //It will return true if the worker is stopping or stopped
        bool isCanceled() const
        {
            return canceled;
        }

        //cancel the worker
        //when you call this function. the worker will be stoped after finish the running job
        void cancel()
        {
            canceled = true;
            parent->data_changed.notify_all();
        }

        int64_t getIdleTimeout() const
        {
            return idleTimeout;
        }

        //set the idle timeout.
        //if the Scheduled does not have new job for some time to do, the worker will be automatically terminated.
        //you can set the value to zero or a very large value
        //the time unit is millisecond
        void setIdleTimeout(int64_t idleTimeout)
        {
            WorkerController::idleTimeout = idleTimeout;
        }

    private:
        int64_t idleTimeout = 1000;

        WorkerController(Scheduled *parent) : parent(parent)
        {
        }

        Scheduled *parent;

    private:
        bool canceled = false;
    };

public:
    Scheduled()
            : work_controller(this)
    {
    }

    ~Scheduled()
    {
        {//Terminate all worker
            std::unique_lock<std::mutex> lk(general_lock);
            workCount = -1;
            data_changed.notify_all();
        }
        if (work_thread)
            work_thread->join();
        set.clear();
    }

    //Just operator delete *_*
    template<typename T>
    static void deleteAnything(T *any)
    {
        delete any;
    }

    template<typename T>
    static callback callback_cast(T func)
    {
        return reinterpret_cast<callback>(func);
    }

public:

    //Schedule Functions
    // schedule_now   - do the task now. A idle worker will start the task at once.
    // schedule_for - do the task some time later. Task will start after a period of time.
    // schedule_at - do the task at the time. Task will start at the time.
    //Arguments:
    // when:
    //      in function *_for, it is how long the delay.
    //      in function *_at, it is the start time.
    // cbTask:
    //      When the time over, the callback function will be called.
    // data:
    //      Scheduled will clone the value, and then the address of new value will be passed to cbTask
    // pData:
    //      The value will be passed to cbTask
    // cbClean:
    //      It will be called when the task object is destructing.
    // cbThrown:
    //      It will be called when cbTask throw something. It will be called in catch segment, so you
    // can use std::current_exception to get some infomation.

    template<typename T>
    std::shared_ptr<TaskNode> schedule_now(callback cbTask,
                                           T const &data,
                                           callback cbThrown = nullptr)
    {
        return schedule_at(std::chrono::time_point<ClockType>(),
                           cbTask,
                           static_cast<void *>(new T(std::forward<T>(data))),
                           reinterpret_cast<void (*)(void *)> (deleteAnything<T>),
                           cbThrown);
    }

    template<typename T, typename T1, typename T2>
    std::shared_ptr<TaskNode> schedule_for(std::chrono::duration<T1, T2> when,
                                           callback cbTask,
                                           T const &data,
                                           callback cbThrown = nullptr)
    {
        return schedule_at(ClockType::now() + when,
                           cbTask,
                           static_cast<void *>(new T(std::forward<T>(data))),
                           reinterpret_cast<void (*)(void *)> (deleteAnything<T>),
                           cbThrown);
    }

    template<typename T>
    std::shared_ptr<TaskNode> schedule_at(std::chrono::time_point<ClockType> when,
                                          callback cbTask,
                                          T const &data,
                                          callback cbThrown = nullptr)
    {
        return schedule_at(when,
                           cbTask,
                           static_cast<void *>(new T(std::forward<T>(data))),
                           reinterpret_cast<void (*)(void *)> (deleteAnything<T>),
                           cbThrown);
    }

    template<typename T>
    std::shared_ptr<TaskNode> schedule_now(callback cbTask,
                                           T &&data,
                                           callback cbThrown = nullptr)
    {
        return schedule_at(std::chrono::time_point<ClockType>(),
                           cbTask,
                           static_cast<void *>(new T(std::forward<T>(data))),
                           reinterpret_cast<void (*)(void *)> (deleteAnything<T>),
                           cbThrown);
    }

    template<typename T, typename T1, typename T2>
    std::shared_ptr<TaskNode> schedule_for(std::chrono::duration<T1, T2> when,
                                           callback cbTask,
                                           T &&data,
                                           callback cbThrown = nullptr)
    {
        return schedule_at(ClockType::now() + when,
                           cbTask,
                           static_cast<void *>(new T(std::forward<T>(data))),
                           reinterpret_cast<void (*)(void *)> (deleteAnything<T>),
                           cbThrown);
    }

    template<typename T>
    std::shared_ptr<TaskNode> schedule_at(std::chrono::time_point<ClockType> when,
                                          callback cbTask,
                                          T &&data,
                                          callback cbThrown = nullptr)
    {
        return schedule_at(when,
                           cbTask,
                           static_cast<void *>(new T(std::forward<T>(data))),
                           reinterpret_cast<void (*)(void *)> (deleteAnything<T>),
                           cbThrown);
    }

    std::shared_ptr<TaskNode> schedule_now(callback cbTask,
                                           void *pData = nullptr,
                                           callback cbClean = nullptr,
                                           callback cbThrown = nullptr)
    {
        return schedule_at(std::chrono::time_point<ClockType>(), cbTask, pData, cbClean, cbThrown);
    }

    template<typename T1, typename T2>
    std::shared_ptr<TaskNode> schedule_for(std::chrono::duration<T1, T2> when,
                                           callback cbTask,
                                           void *pData = nullptr,
                                           callback cbClean = nullptr,
                                           callback cbThrown = nullptr)
    {
        return schedule_at(ClockType::now() + when, cbTask, pData, cbClean, cbThrown);
    }


    std::shared_ptr<TaskNode> schedule_at(std::chrono::time_point<ClockType> when,
                                          callback cbTask,
                                          void *pData = nullptr,
                                          callback cbClean = nullptr,
                                          callback cbThrown = nullptr)
    {
        std::unique_lock<std::mutex> lk(general_lock);
        std::shared_ptr<TaskNode>
                spTask = std::shared_ptr<TaskNode>(new TaskNode(this, when, cbTask, pData));
        spTask->throwCallback = cbThrown;
        mset_const_iter iter = set.insert(spTask);
        if (workCount > 0)
        {
            if (iter == set.begin())
                data_changed.notify_all();
        }
        else
        {
            if (work_thread)
            {
                if (work_thread->joinable())
                    work_thread->detach();//...
                delete work_thread;
            }
            work_thread = new std::thread(&Scheduled::worker_routine, this, &work_controller);
        }
        spTask->cleanCallback = cbClean;
        return spTask;
    }

    //remove a task, the implement of TaskNode::cancel
    bool remove(TaskNode const*task)
    {
        std::unique_lock<std::mutex> lk(general_lock);
        bool found = false;
        mset_const_iter iter;
        for (mset_const_iter ci = set.cbegin(); ci != set.cend(); ++ci)
        {
            if ((*ci).get() == task)
            {
                found = true;
                iter = ci;
            }
        }
        if (found)
        {
            if (iter == set.cbegin())
                data_changed.notify_all();

            set.erase(iter);
        }
        return found;
    }

    //Use the currect thread as a worker.
    //if pController is not nullptr , it will be set to std::weak_ptr<WorkerController>
    //you can use the WorkerController to focus the function exit
    void join(std::weak_ptr<WorkerController> *pController = nullptr)
    {
        WorkerController *pcb = new WorkerController(this);
        std::shared_ptr<WorkerController> spcb(pcb);
        if (pController)
            *pController = std::weak_ptr<WorkerController>(spcb);
        worker_routine(pcb);
    }

private:
    void worker_routine(WorkerController *cb)
    {
        ++workCount;
        while (workCount > 0 && !cb->canceled)
        {
            std::unique_lock<std::mutex> lk(general_lock);
            if (set.empty())
            {
                if (std::cv_status::timeout ==
                    data_changed.wait_for(lk, std::chrono::milliseconds(cb->idleTimeout)))
                {
                    cb->canceled = true;
                    continue;
                }
                if (set.empty()) continue;
            }
            mset_const_iter iter = set.cbegin();
            if (data_changed.wait_until(lk, (*iter)->time, [iter, this] {
                return iter != set.begin() || workCount <= 0;
            }))
            {
                continue;
            }
            std::shared_ptr<TaskNode> task = *iter;
            set.erase(iter);
            lk.unlock();
            task->run();
        }
        --workCount;
    }


private:
    std::mutex general_lock;
    std::condition_variable data_changed;
    std::multiset<std::shared_ptr<TaskNode>, TaskCompare> set;
    std::thread *work_thread = nullptr;
    WorkerController work_controller;
    int workCount = 0;
};

#endif