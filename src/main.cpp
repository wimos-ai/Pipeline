#include "PipelineStage.hpp"
#include "MTPrimitives.hpp"

#include <iostream>

int main()
{
    SPSCQ<int> queue;
    queue.add_item(1);
    std::cout << queue.size() << '\n';
    std::cout << queue.get_item().value() << '\n';
    std::cout << queue.size() << '\n';

    // PipelineAction add_one{[](int i) -> int
    //                        { return i + 1; }};
    // PipelineAction pow_two{[](int i) -> int
    //                        { return i * i; }};

    // auto add_three = [](int i)
    // { return i + 3; };

    // auto combined_stage = add_one + add_one + add_one + pow_two;

    // auto add_four = add_one + add_three;

    // std::cout << "combined_stage.get_item().value()";
}