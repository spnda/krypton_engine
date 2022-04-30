#include <Tracy.hpp>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
// We disable clang-format as the order of these headers matters!
// clang-format off
#include <Windows.h>
#include <processthreadsapi.h>
// clang-format on
#endif

#include <util/logging.hpp>
#include <util/scheduler.hpp>

namespace kt = krypton::threading;

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

[[maybe_unused]] uint32_t kt::Scheduler::getMaxThreadCount() const {
    return maxThreadCount;
}

void kt::Scheduler::run(const kt::Scheduler::taskFunction& function) {
    ZoneScoped;
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
    ZoneScoped;
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
    ZoneScoped;
    auto lock = std::scoped_lock(threadPoolMutex);

    krypton::log::log("Launching thread pool with {} threads", maxThreadCount);

    if (threadPool.capacity() != maxThreadCount) {
        // Possible that this scheduler has never been used. We reserve
        // enough threads to saturate every available thread.
        threadPool.reserve(maxThreadCount);
    }

    for (uint32_t i = 0; i < maxThreadCount; ++i) {
        threadPool.emplace_back(&Scheduler::workerThreadLoop, this, i);
    }

    running = true;
}

void kt::Scheduler::workerThreadLoop(uint32_t threadId) {
    {
        auto threadName = fmt::format("Worker Thread {}", threadId);
        tracy::SetThreadName(threadName.c_str());

#if defined(__APPLE__) || defined(__linux__)
        pthread_setname_np(threadName.c_str());
#elif WIN32
        auto wthreadName = std::wstring(threadName.begin(), threadName.end());
        SetThreadDescription(threadPool.back().native_handle(), wthreadName.c_str());
#endif
    }

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
            ZoneScoped job();
        }
    }
}
