# cppThreadPool
C++ thread pool class

A **Thread Pool** is a critical design pattern in concurrent programming used to manage and reuse a fixed number of threads to execute tasks asynchronously. Instead of creating and destroying a thread for every single task—which is computationally expensive—a thread pool maintains a queue of tasks and a set of worker threads that stay alive for the duration of the application.

Below is a comprehensive guide and implementation of a modern C++ Thread Pool using **C++17/C++20** standards.

## Why Use a Thread Pool?

1.  **Performance**: Creating a thread involves significant overhead (stack allocation, kernel calls). Reusing threads eliminates this cost.
2.  **Resource Management**: It prevents the system from being overwhelmed by too many threads, which can lead to excessive context switching and memory exhaustion.
3.  **Task Decoupling**: It separates the submission of a task from its execution logic.

## Core Components

A robust C++ Thread Pool typically consists of:
*   **Worker Threads**: A vector of `std::thread` objects that loop indefinitely, waiting for tasks.
*   **Task Queue**: A `std::queue` holding functions (usually `std::function<void()>`) to be executed.
*   **Synchronization**: 
    *   `std::mutex` to protect the task queue from race conditions.
    *   `std::condition_variable` to notify worker threads when a new task is available or when the pool is shutting down.
*   **Future/Promise Mechanism**: To allow the caller to retrieve results from an asynchronous task.
