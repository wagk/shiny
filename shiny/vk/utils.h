#pragma once

#include <vector>

namespace shiny::vk {

template<typename R, typename F, typename H>
std::vector<R>
collect(F func, const H& handle)
{
    uint32_t count = 0;
    func(handle, &count, nullptr);

    std::vector<R> vec(count);
    func(handle, &count, vec.data());

    return vec;
}

template<typename R, typename F>
std::vector<R>
collect(F func)
{
    uint32_t count = 0;
    func(&count, nullptr);

    std::vector<R> vec(count);
    func(&count, vec.data());

    return vec;
}

}  // namespace shiny::vk
