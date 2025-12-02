#include <thread>
#include <future>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <type_traits>
#include <exception>

#ifndef THREADS_C
#define THREADS_C

class threadPool {
public:
    threadPool(size_t num_threads) : stop(false) {
        threads.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this] { workerLoop(); });
        }
    }

    ~threadPool() {
        {
            std::unique_lock lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& thread : threads) {
            thread.join();
        }
    }

    template <typename F, typename... Args>
    auto addTask(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        using TaskType = std::packaged_task<ReturnType()>;
        
        TaskType task([func = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
            return func(std::forward<Args>(args)...);
        });
        
        auto future = task.get_future();
        {
            std::unique_lock lock(queue_mutex);
            if (stop) throw std::runtime_error("ThreadPool stopped");
            tasks.emplace_back(std::move(task));
        }
        condition.notify_one();
        return future;
    }

    // Batch task addition
    template <typename F, typename... Args>
    auto addTaskBatch(size_t count, F&& f, std::vector<Args&&...> args) -> std::vector<std::future<std::invoke_result_t<F, Args...>>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        std::vector<std::future<ReturnType>> futures;
        futures.reserve(count);
        
        {
            std::unique_lock lock(queue_mutex);
            if (stop) throw std::runtime_error("ThreadPool stopped");
            
            for (size_t i = 0; i < count; ++i) {
                auto task = std::packaged_task<ReturnType()>(
                    [func = std::forward<F>(f), ...args = std::forward<Args>(args[i])]() mutable {
                        return func(std::forward<Args>(args)...);
                    });
                    
                futures.emplace_back(task.get_future());
                tasks.emplace_back(std::move(task));
            }
        }
        
        condition.notify_all();
        return futures;
    }

    threadPool(const threadPool&) = delete;
    threadPool& operator=(const threadPool&) = delete;

private:
    struct TaskWrapper {
        struct Base {
            virtual ~Base() = default;
            virtual void execute() = 0;
        };

        template<typename T>
        struct Wrapper : Base {
            T task;
            explicit Wrapper(T&& t) : task(std::move(t)) {}
            void execute() override { task(); }
        };

        std::unique_ptr<Base> wrapper;

        TaskWrapper() = default;
        
        template<typename T>
        TaskWrapper(T&& task) 
            : wrapper(new Wrapper<T>(std::forward<T>(task))) {}
        
        TaskWrapper(TaskWrapper&&) noexcept = default;
        TaskWrapper& operator=(TaskWrapper&&) noexcept = default;
        
        void operator()() { wrapper->execute(); }
    };

    void workerLoop() {
        while (true) {
            TaskWrapper task;
            {
                std::unique_lock lock(queue_mutex);
                condition.wait(lock, [this] { return stop || !tasks.empty(); });
                
                if (stop && tasks.empty()) return;
                
                task = std::move(tasks.front());
                tasks.pop_front();
            }
            
            try {
                task();
            } catch (...) {
                // Exception handling logic
            }
        }
    }

    bool stop = false;
    
    std::vector<std::thread> threads;
    std::deque<TaskWrapper> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
};

#endif
