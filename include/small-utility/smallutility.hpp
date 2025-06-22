/* UTF-8 */
/**
 *@file smallutility.hpp
 *@brief 实现一些自定义小功能
 */

#ifndef SMALLUTILITY_HPP
#define SMALLUTILITY_HPP
#include <concepts>
#include <memory>
#include <type_traits>

namespace user {
template <std::forward_iterator Iterator>
constexpr Iterator move_or_copy(
    Iterator first, Iterator last, Iterator result
) {
    using value_type = typename std::iterator_traits<Iterator>::value_type;

    if constexpr (std::is_move_constructible_v<value_type>) {
        // 将原来内存区域的值移动到新区域
        return std::uninitialized_move(first, last, result);
    } else if constexpr (std::is_copy_constructible_v<value_type>) {
        // 如果不可移动但是可以复制
        return std::uninitialized_copy(first, last, result);
    } else {
        // 既不可移动也不可复制，则抛出异常
        throw std::runtime_error(
            "M_realloc_insert: The type of the value is unable to copy "
            "or move"
        );
    }
}
}  // namespace user

#endif  // SMALLUTILITY_HPP
