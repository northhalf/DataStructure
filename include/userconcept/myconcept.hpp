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

/**
 * @concept SameTypeAlloc
 * @brief 判断数值类型是否与分配器匹配
*/
template<typename Tp, typename Alloc>
concept SameTypeAlloc = std::is_same_v<Tp, typename Alloc::value_type>;

/**
 * @concept NotConstVolatile
 * @brief 非const和volatile类型
*/
template<typename Tp>
concept NotConstVolatile = std::is_same_v<typename std::remove_cv_t<Tp>, Tp>;

}  // namespace user
#endif  // MYCONCEPT_HPP
