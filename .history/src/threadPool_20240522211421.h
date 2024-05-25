#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool
{
public:
    ThreadPool(size_t);
    ~ThreadPool();

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// Definition of the template member function
/**
 * @brief Enqueues a function to be executed by one of the worker threads in the pool.
 *
 * @tparam F The type of the function to be executed.
 * @tparam Args The types of the arguments to be passed to the function.
 * @param f The function to be executed.
 * @param args The arguments to be passed to the function.
 *
 * @return A future object that can be used to retrieve the result of the function once it has completed.
 *
 * @throws std::runtime_error If the thread pool has been stopped and no more tasks can be enqueued.
 *
 * @note This function is thread-safe and can be called from multiple threads simultaneously.
 */
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    // Create a packaged_task that wraps the function and its arguments
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    // Get a future object associated with the packaged_task
    std::future<return_type> res = task->get_future();

    {
        // Lock the queue mutex to ensure thread safety
        std::unique_lock<std::mutex> lock(queue_mutex);

        // Check if the thread pool has been stopped
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        // Add the packaged_task to the task queue
        tasks.emplace([task]()
                      { (*task)(); });
    }

    // Notify one of the worker threads that a new task is available
    condition.notify_one();

    // Return the future object associated with the packaged_task
    return res;
}

#endif // THREADPOOL_H
