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
/**
 * @brief 实现根据元素类型判断执行移动还是复制初始化操作
 * @tparam InputIt 输入迭代器
 * @tparam ForwardIt 前向迭代器
 * @param first 原来区域的第一个迭代器
 * @param last 原来区域的最后一个元素的后一个迭代器
 * @param result 指向目标区域的第一个位置的迭代器
 * @return result + (last - first)， 即result迭代器移动到的位置
 */
template <std::input_iterator InputIt, std::forward_iterator ForwardIt>
constexpr ForwardIt uninitialized_move_or_copy(
    InputIt first, InputIt last, ForwardIt result
) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;

    if constexpr (std::is_move_constructible_v<value_type>) {
        // 将原来内存区域的值移动到新区域
        return std::uninitialized_move(first, last, result);
    } else if constexpr (std::is_copy_constructible_v<value_type>) {
        // 如果不可移动但是可以复制
        return std::uninitialized_copy(first, last, result);
    } else {
        // 既不可移动也不可复制，则抛出异常
        throw std::runtime_error(
            "move_or_copy: The type of the value is unable to copy "
            "or move"
        );
    }
}

/**
 * @brief 根据能否移动或复制将原来区域的值移动到新区域
 * @tparam BidirIt1 第一个双向迭代器类型
 * @tparam BidirIt2 第二个双向迭代器类型
 * @param first 指向原来区域的第一个值
 * @param last 指向原来区域的最后一个值的后一个位置
 * @param result 结果区域的最后一个值的后一个位置
 * @return result-(last-first), result迭代器的结果值，指向结果区域的第一个值
 */
template <
    std::bidirectional_iterator BidirIt1, std::bidirectional_iterator BidirIt2>
constexpr BidirIt2 move_or_copy_backward(
    BidirIt1 first, BidirIt1 last, BidirIt2 result
) {
    using value_type = typename std::iterator_traits<BidirIt1>::value_type;

    if constexpr (std::is_move_constructible_v<value_type>) {
        // 将原来区域的值移动到新区域
        return std::move_backward(first, last, result);
    } else if constexpr (std::is_copy_constructible_v<value_type>) {
        // 如果不可移动但是可以复制
        return std::copy_backward(first, last, result);
    } else {
        // 既不可移动也不可复制，则抛出异常
        throw std::runtime_error(
            "move_or_copy_backward: The type of the value is unable to copy "
            "or move"
        );
    }
}

}  // namespace user

#endif  // SMALLUTILITY_HPP
