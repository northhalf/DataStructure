/* UTF-8 */
/**
 * @file my-allocator.hpp
 * @brief user::Allocator类
 */

#ifndef MY_ALLOCATOR_HPP
#define MY_ALLOCATOR_HPP 2

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace user {

/**
 * @class Allocator
 * @brief 自定义容器分配器
 * @details 使用allocate()分配内存和deallocate()释放内存
 */
template <typename T>
class Allocator {
public:
    // C++20 标准规定的类型成员
    // 数值类型
    using value_type = T;
    // 内存分配的内存块尺寸信息类型
    using size_type = std::size_t;
    // 两指针之间距离类型
    using difference_type = std::ptrdiff_t;
    // 容器移动赋值时分配器跟随移动
    using propagate_on_container_move_assignment = std::true_type;

    Allocator() = default;
    virtual ~Allocator() = default;

    /**
     * @brief 分配给对象分配内存
     * @note 仅分配内存，不进行初始化操作
     * @param n 需要分配内存的对象数目
     * @return T* 指向对应内存区域的指针
     */
    constexpr T* allocate(size_type n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    /**
     * @brief 释放对象内存
     * @param p 指向需要释放的内存区域的指针
     */
    constexpr void deallocate(T* p, size_type) { ::operator delete(p); }
};

/**
 * @brief 分配器==函数
 * @note 因为分配器总是一样的，所以只返回true
 * @return true
 */
template <typename T1, typename T2>
inline constexpr bool operator==(
    const Allocator<T1>& lhs, const Allocator<T2> rhs
) noexcept {
    return true;
}

}  // namespace user

#endif