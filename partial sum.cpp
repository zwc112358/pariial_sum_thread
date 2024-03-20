﻿// partial sum.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <vector>
#include <future>
#include <thread>
#include <numeric>


class join_threads {
    std::vector<std::thread>& threads; // 引用线程向量
public:
    explicit join_threads(std::vector<std::thread>& threads_) // 构造函数
        : threads(threads_) {}

    ~join_threads() { // 析构函数，在析构时等待所有线程结束
        for (std::size_t i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }
};

template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last)
{

    typedef typename Iterator::value_type value_type;

    struct process_chunk  // 1
    {
        void operator()(Iterator begin, Iterator last,
            std::future<value_type>* previous_end_value,
            std::promise<value_type>* end_value)
        {
            std::cout << "current thread id:" << std::this_thread::get_id() << std::endl;
            try
            {
                Iterator end = last;
                ++end;
                std::partial_sum(begin, end, begin);  // 2
                if (previous_end_value)  // 3
                {
                    auto addend = previous_end_value->get();  // 4
                    *last += addend;  // 5
                    if (end_value)
                    {
                        end_value->set_value(*last);  // 6
                    }
                    std::for_each(begin, last, [addend](value_type& item)  // 7
                    {
                        item += addend;
                    });
                }
                else if (end_value)
                {
                    end_value->set_value(*last);  // 8
                }
            }
            catch (...)  // 9
            {
                if (end_value)
                {
                    end_value->set_exception(std::current_exception());  // 10
                }
                else
                {
                    throw;  // 11
                }
            }
        }
    };

    unsigned long const length = std::distance(first, last);

    if (!length)
        return;

    unsigned long const min_per_thread = 25;  // 12
    unsigned long const max_threads =
        (length + min_per_thread - 1) / min_per_thread;

    unsigned long const hardware_threads =
        std::thread::hardware_concurrency();

    unsigned long const num_threads =
        std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

    unsigned long const block_size = length / num_threads;

    typedef typename Iterator::value_type value_type;

    std::vector<std::thread> threads(num_threads - 1);  // 13
    std::vector<std::promise<value_type> >
        end_values(num_threads - 1);  // 14
    std::vector<std::future<value_type> >
        previous_end_values;  // 15
    previous_end_values.reserve(num_threads - 1);  // 16
    join_threads joiner(threads);


    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_last = block_start;
        std::advance(block_last, block_size - 1);  // 17
        threads[i] = std::thread(process_chunk(),  // 18
            block_start, block_last,
            (i != 0) ? &previous_end_values[i - 1] : 0,
            &end_values[i]);
        block_start = block_last;
        ++block_start;  // 19
        previous_end_values.push_back(end_values[i].get_future());  // 20
    }
    Iterator final_element = block_start;
    std::advance(final_element, std::distance(block_start, last) - 1);  // 21
    process_chunk()(block_start, final_element,  // 22
        (num_threads > 1) ? &previous_end_values.back() : 0,
        0);
}
template<typename Iterator>
void print(Iterator first, Iterator last)
{
    auto iter = first;
    for (; iter != last; ++iter)
    {
        std::cout << *iter << "\t";
    }
    std::cout << std::endl;
}
int main()
{

    std::vector<int> vec;// = { 1,2,3,4,5,6 };
    for (int i = 1; i < 100; ++i)
    {
        vec.push_back(i);
    }
    print(vec.begin(), vec.end());
    
    parallel_partial_sum(vec.begin(), vec.end());
    print(vec.begin(), vec.end());
    return 0;
}