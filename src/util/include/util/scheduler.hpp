#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace krypton::threading {
    // Task-based thread scheduler using a singleton so that jobs can be
    // dispatched from anywhere.
    class Scheduler final {
        using taskFunction = std::function<void()>;
        using workerThreadFunction = std::function<void(Scheduler*)>;

        // We subtract by one as our main thread is not allocated through
        // this scheduler and we still want it to run concurrently.
        uint32_t maxThreadCount = std::thread::hardware_concurrency() - 1;

        std::vector<std::thread> threadPool = {};
        mutable std::mutex threadPoolMutex;

        // This helps us notify worker threads about new work. This is
        // useful so the thread doesn't just continuosly run waiting for
        // work to do.
        std::condition_variable cv;

        std::deque<taskFunction> pendingTasks = {};
        mutable std::mutex taskMutex;

        // This indicates whether we're shutting down and no new jobs can
        // be started. We don't use std::atomic because this should only
        // be called from the "host".
        bool terminate = false;

        bool running = false;

        // The function that gets executed by each worker thread in which
        // they wait on new work by listening to the std::condition_variable.
        void workerThreadLoop(uint32_t threadId);

        explicit Scheduler();

    public:
        ~Scheduler();

        static auto getInstance() -> Scheduler&;
        [[maybe_unused]] auto getMaxThreadCount() const -> uint32_t;
        void run(const Scheduler::taskFunction& function);
        void shutdown();
        void start();
    };
} // namespace krypton::threading
