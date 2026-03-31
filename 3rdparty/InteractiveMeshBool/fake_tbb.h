#pragma once
#include <condition_variable>
#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>

namespace
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(size_t num_threads) : stop_(false)
        {
            for (size_t i = 0; i < num_threads; ++i)
            {
                workers_.emplace_back([this]() {
                    while (true)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(mutex_);
                            cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                            if (stop_ && tasks_.empty()) return;
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        task();
                    }
                    });
            }
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                stop_ = true;
            }
            cv_.notify_all();
            for (auto& t : workers_) t.join();
        }

        void enqueue(std::function<void()> task)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                tasks_.push(std::move(task));
            }
            cv_.notify_one();
        }

        size_t size() const { return workers_.size(); }

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool stop_;
    };

    template<typename Index, typename Func>
    void parallel_for(Index first, Index last, Func func, size_t num_threads = 0)
    {
        if (first >= last) return;
        if (num_threads == 0) num_threads = std::thread::hardware_concurrency();
        const size_t n = static_cast<size_t>(last - first);
        const size_t chunk_size = (n + num_threads - 1) / num_threads;

        ThreadPool pool(num_threads);
        std::atomic<size_t> completed{ 0 };
        std::mutex mutex;
        std::condition_variable cv;
        for (size_t t = 0; t < num_threads; ++t)
        {
            Index chunk_begin = first + static_cast<Index>(t * chunk_size);
            Index chunk_end = std::min(chunk_begin + static_cast<Index>(chunk_size), last);
            if (chunk_begin >= chunk_end) break;

            pool.enqueue([=, &completed, &mutex, &cv]() {
                for (Index i = chunk_begin; i < chunk_end; ++i) func(i);
                if (++completed == num_threads)
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    cv.notify_one();
                }
                });
        }

        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return completed == num_threads; });
    }

    class TaskGroup
    {
    public:
        TaskGroup() : active_(0), stop_(false) {}
        ~TaskGroup() { wait(); }

        template<typename Func>
        void run(Func&& f)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++active_;
            }

            auto future = std::async(std::launch::async, [this, f]() mutable {
                try { f(); }
                catch (...)
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    exceptions_.push_back(std::current_exception());
                }

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (--active_ == 0) { cv_.notify_all(); }
                }
            });

            std::lock_guard<std::mutex> lock(mutex_);
            futures_.push_back(std::move(future));
        }

        void wait()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return active_ == 0; });

            lock.unlock();
            for (auto& f : futures_) { if (f.valid()) f.get(); }
            futures_.clear();

            lock.lock();
            if (!exceptions_.empty())
            {
                auto e = exceptions_.front();
                exceptions_.clear();
                std::rethrow_exception(e);
            }
        }

        bool is_empty() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return active_ == 0;
        }

    private:
        std::atomic<size_t> active_;
        std::vector<std::future<void>> futures_;
        std::vector<std::exception_ptr> exceptions_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        bool stop_;
    };

    class TaskGroupPool {
    public:
        explicit TaskGroupPool(size_t num_threads = 0)
            : pool_(num_threads == 0 ? std::thread::hardware_concurrency() : num_threads)
            , active_(0) {}

        template<typename Func>
        void run(Func&& f)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++active_;
            }

            pool_.enqueue([this, f]() mutable {
                try { f(); }
                catch (...)
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    exceptions_.push_back(std::current_exception());
                }

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (--active_ == 0) { cv_.notify_all(); }
                }
            });
        }

        void wait()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return active_ == 0; });

            if (!exceptions_.empty())
            {
                auto e = exceptions_.front();
                exceptions_.clear();
                std::rethrow_exception(e);
            }
        }

    private:
        ThreadPool pool_;
        std::atomic<size_t> active_;
        std::vector<std::exception_ptr> exceptions_;
        std::mutex mutex_;
        std::condition_variable cv_;
    };
}
