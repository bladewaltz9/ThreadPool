#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

#include "thread_safe_queue.h"

class ThreadPool {
public:
    ThreadPool(const int num_threads = 4)
        : m_threads(std::vector<std::thread>(num_threads)), m_shutdown(false) {
    }

    ThreadPool(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool() {}

public:
    void init() {
        for (int i = 0; i < m_threads.size(); ++i) {
            m_threads[i] = std::thread(ThreadWorker(this, i));
        }
    }

    void shutdown() {
        m_shutdown = true;
        m_condition.notify_all();  // TODO: 这里通知后，dequeue 会不会有问题？

        for (int i = 0; i < m_threads.size(); ++i) {
            if (m_threads[i].joinable()) {
                m_threads[i].join();
            }
        }
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        // create a function with bounded parameter ready to execute
        // bind: bind function and parameter
        // forward: maintain the left or right value character of paramater
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        // wrap packaged task into void function
        std::function<void()> warpper_func = [task_ptr]() {
            (*task_ptr)();
        };

        m_queue.enqueue(warpper_func);

        m_condition.notify_one();

        return task_ptr->get_future();
    }

private:
    class ThreadWorker {  // Built-in thread worker class
    public:
        ThreadWorker(ThreadPool* pool, int id)
            : m_pool(pool), m_id(id) {}

        // overload () operation, use object() to call
        void operator()() {
            std::function<void()> func;

            bool dequeued;  // whether dequeue successful

            while (!m_pool->m_shutdown) {
                // lock for conditional variable
                std::unique_lock<std::mutex> lock(m_pool->m_condition_mutex);

                if (m_pool->m_queue.empty()) {
                    m_pool->m_condition.wait(lock);  // wait conditional variable notify
                }

                dequeued = m_pool->m_queue.dequeue(func);

                // if dequeue successfully, call worker function
                if (dequeued) func();
            }
        }

    private:
        int         m_id;    // worker id
        ThreadPool* m_pool;  // thread pool which it belongs
    };

private:
    bool m_shutdown;                                   // whether shutdown thread pool

    ThreadSafeQueue<std::function<void()>> m_queue;    // tasks queue
    std::vector<std::thread>               m_threads;  // work threads queue
    std::mutex                             m_condition_mutex;
    std::condition_variable                m_condition;
};

#endif