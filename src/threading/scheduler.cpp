#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#endif

#include <threading/scheduler.hpp>
#include <util/logging.hpp>

kt::Scheduler::Scheduler() {
    // We may only have a single core processor or
    // the thread count was unable to be determined
    // and we have to have *at least* 1 worker thread.
    if (maxThreadCount <= 0) {
        maxThreadCount = 1;
    }
}

kt::Scheduler::~Scheduler() {
    if (running) {
        shutdown();
    }
}

kt::Scheduler& kt::Scheduler::getInstance() {
    static kt::Scheduler instance;
    return instance;
}

uint32_t kt::Scheduler::getMaxThreadCount() { return maxThreadCount; }

void kt::Scheduler::run(kt::Scheduler::taskFunction function) {
    if (!running) {
        return;
    }

    auto guard = std::scoped_lock(taskMutex);
    pendingTasks.emplace_back(function);

    // We simply notify one worker thread that they can work
    // on our job.
    cv.notify_one();
}

void kt::Scheduler::shutdown() {
    terminate = true;

    // Wake up all threads
    cv.notify_all();

    for (auto& thread : threadPool) {
        thread.join();
    }

    threadPool.clear();

    running = false;
}

void kt::Scheduler::start() {
    auto lock = std::scoped_lock(threadPoolMutex);

    krypton::log::log("Launching thread pool with {} threads", maxThreadCount);

    if (threadPool.capacity() != maxThreadCount) {

        // Possible that this scheduler has never been used. We reserve
        // enough threads to saturate every available thread.
        threadPool.reserve(maxThreadCount);
    }

    for (uint32_t i = 0; i < maxThreadCount; ++i) {
        threadPool.push_back(std::thread(&Scheduler::workerThreadLoop, this));
#ifdef WIN32
        auto threadName = fmt::format("Worker thread {}", i);
        std::wstring wthreadName = std::wstring(threadName.begin(), threadName.end());
        SetThreadDescription(threadPool.back().native_handle(), wthreadName.c_str());
#endif
    }

    running = true;
}

void kt::Scheduler::workerThreadLoop() {
    // This thread is supposed to run infinitely.
    while (true) {
        kt::Scheduler::taskFunction job;

        // We create a scope so the scoped_lock gets automatically destroyed
        // when we're done.
        {
            auto lock = std::unique_lock(taskMutex);

            // This essentially halts this thread without burning CPU.
            cv.wait(lock, [this]() { return !pendingTasks.empty() || terminate; });

            // We're shutting down!
            if (pendingTasks.empty() && terminate)
                break;

            // We will (for now) just take the first available job and pop it
            // Potentially, we could search for specific jobs based on capabilities,
            // context or dependencies.
            job = std::move(pendingTasks.front());
            pendingTasks.pop_front();
        }

        // Now, execute the job.
        if (job) {
            job();
        }
    }
}
