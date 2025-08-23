// ThreadPool.cpp
#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t num_threads) : stop(false), active_tasks(0) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    for (std::thread &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(std::move(task));
        ++active_tasks;
    }
    condition.notify_one();
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    condition.wait(lock, [this]() {
        return tasks.empty() && active_tasks.load() == 0;
    });
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this]() {
                return stop.load() || !tasks.empty();
            });

            if (stop.load() && tasks.empty()) return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            --active_tasks;
        }
        condition.notify_all();
    }
}
