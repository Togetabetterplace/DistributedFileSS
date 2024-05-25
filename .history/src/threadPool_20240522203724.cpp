#include "ThreadPool.h"

// Constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            [this]
            {
                for (;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                                             [this]
                                             { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
            });
}

// The destructor joins all threads
/**
 * @brief Destructor for ThreadPool class.
 *
 * The destructor ensures that all threads are properly joined before the object is destroyed.
 * It sets the stop flag to true, notifies all waiting threads, and then waits for each thread to finish.
 */
ThreadPool::~ThreadPool()
{
    {
        // Lock the queue mutex to ensure exclusive access to the tasks queue and stop flag
        std::unique_lock<std::mutex> lock(queue_mutex);

        // Set the stop flag to true to indicate that the thread pool should stop accepting new tasks
        stop = true;
    }

    // Notify all waiting threads that the stop flag has been set
    condition.notify_all();

    // Join each worker thread to ensure they have finished executing their tasks
    for (std::thread &worker : workers)
        worker.join();
}
