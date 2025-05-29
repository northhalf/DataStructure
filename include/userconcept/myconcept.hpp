/**
 * @file myconcept.hpp
 * @brief 自定义的用于模板的concept
 */

#ifndef MYCONCEPT_HPP
#define MYCONCEPT_HPP
#include <concepts>
#include <type_traits>

namespace user {

/**
 * @concept IsAllocator
 * @brief 判断类型是否为分配器
 */
template <typename Tp>
concept IsAllocator = requires(Tp alloc) {
    // 要求拥有allocator方法和value_type类型，并且方法返回对应指针
    { alloc.allocate(1) } -> std::same_as<typename Tp::value_type*>;

    // 要求具有deallocate方法，返回void值
    {
        alloc.deallocate(std::declval<typename Tp::value_type*>(), 1)
    } -> std::same_as<void>;
};

}  // namespace user
#endif  // MYCONCEPT_HPP
