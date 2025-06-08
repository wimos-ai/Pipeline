#pragma once

#include <mutex>
#include <queue>
#include <optional>
#include <semaphore>

template <typename T>
class SPSCQ
{
public:
    SPSCQ() : mtx{}, queue{}, cs{0} {}

    std::optional<T> get_item()
    {
        this->cs.acquire();
        std::unique_lock l{mtx};
        if (this->queue.size() == 0)
        {
            return std::nullopt;
        }
        T top = this->queue.front();
        this->queue.pop();
        return top;
    }

    size_t size()
    {
        std::unique_lock l{mtx};
        return this->queue.size();
    }

    void add_item(T &t)
    {
        std::unique_lock l{mtx};
        this->queue.emplace(t);
        this->cs.release();
    }

    void add_item(T t)
    {
        std::unique_lock l{mtx};
        this->queue.emplace(t);
        this->cs.release();
    }

    void notify()
    {
        this->cs.release();
    }

private:
    std::mutex mtx;
    std::queue<T> queue;
    std::counting_semaphore<> cs;
};