#pragma once

#include <vector>
#include <queue>
#include <functional>
#include <optional>
#include <thread>
#include <memory>

#include "MTPrimitives.hpp"

int test();

template <typename T>
struct function_traits;

// For function pointers
template <typename R, typename Arg>
struct function_traits<R (*)(Arg)>
{
    using input_type = Arg;
    using output_type = R;
};

// For std::function
template <typename R, typename Arg>
struct function_traits<std::function<R(Arg)>>
{
    using input_type = Arg;
    using output_type = R;
};

// For lambdas and functors
template <typename T>
struct function_traits
{
private:
    using call_type = function_traits<decltype(&T::operator())>;

public:
    using input_type = typename call_type::input_type;
    using output_type = typename call_type::output_type;
};

// For lambda operator()
template <typename C, typename R, typename Arg>
struct function_traits<R (C::*)(Arg) const>
{
    using input_type = Arg;
    using output_type = R;
};

template <typename I, typename O, typename Fn>
class PipelineAction
{
public:
    PipelineAction(Fn fn) : fn(fn) {}

    Fn get_fn()
    {
        return this->fn;
    }

    O operator()(I &i)
    {
        return fn(i);
    }

private:
    Fn fn;
};

// 3. Add a deduction guide:
template <typename Fn>
PipelineAction(Fn)
    -> PipelineAction<
        typename function_traits<Fn>::input_type,
        typename function_traits<Fn>::output_type,
        Fn>;

template <typename I, typename O, typename Fn>
struct function_traits<PipelineAction<I, O, Fn>>
{
    using input_type = I;
    using output_type = O;
};

template <typename C1, typename R1, typename Arg1, typename C2, typename R2, typename Arg2>
auto operator+(PipelineAction<C1, R1, Arg1> a, PipelineAction<C2, R2, Arg2> b)
{
    auto fn1 = a.get_fn();
    auto fn2 = b.get_fn();
    auto combined_fn = [fn1, fn2](C1 v) -> R2
    {
        auto intermediate_value = fn1(v);
        return fn2(intermediate_value);
    };

    return PipelineAction(combined_fn);
};

template <typename C1, typename R1, typename Arg1, typename Fn>
auto operator+(PipelineAction<C1, R1, Arg1> a, Fn b)
{
    PipelineAction b_{b};
    PipelineAction rv = a + b_;
    return rv;
};

template <typename C1, typename R1, typename Arg1, typename Fn>
auto operator+(Fn a, PipelineAction<C1, R1, Arg1> b)
{
    PipelineAction a_{a};
    PipelineAction rv = a_ + b;
    return rv;
};

template <typename I, typename O, typename Fn>
class PipelineStep
{
public:
    using Action = PipelineAction<I, O, Fn>;
    using InputQueue = SPSCQ<I>;
    using OutputQueue = SPSCQ<O>;

    PipelineStep(std::shared_ptr<InputQueue> in,
                 std::shared_ptr<OutputQueue> out,
                 Action action)
        : input(in), output(out), fn(action), running(true),
          worker(&PipelineStep::run, this)
    {
    }

    ~PipelineStep()
    {
        stop();
    }

    void stop()
    {
        running.store(false);
        if (worker.joinable())
        {
            worker.join();
        }
    }

private:
    std::shared_ptr<InputQueue> input;
    std::shared_ptr<OutputQueue> output;
    Action fn;
    std::atomic<bool> running;
    std::thread worker;

    void run()
    {
        while (running.load())
        {
            I item;
            if (input->try_pop(item))
            {
                O result = fn(item);
                while (!output->try_push(result) && running.load())
                {
                    // Optionally sleep or yield here
                    std::this_thread::yield();
                }
            }
            else
            {
                std::this_thread::yield(); // Back off if no input
            }
        }
    }
};
