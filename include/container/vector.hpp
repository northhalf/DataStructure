
/**
 * @file vector.hpp
 * @brief 实现自定义的Vector类实现
 */

#ifndef VECTOR_HPP
#define VECTOR_HPP
#include <concepts>
#include <container/vectorbase.hpp>
#include <iterator>
#include <limits>
#include <memory>
#include <userconcept/myconcept.hpp>
namespace user {

/**
 * @class Vector
 * @brief 实现可变数组
 * @tparam Tp 要存储的数据类型，不可为const或valatile修饰类型
 * @tparam Alloc 分配器类型，分配的类型需要与Tp相同
 */
template <NotConstVolatile Tp, IsAllocator Alloc = std::allocator<Tp>>
    requires SameTypeAlloc<Tp, Alloc>  // 要求分配器类型与数值类型匹配
class Vector : protected VectorBase<Tp, Alloc> {
    using Base = VectorBase<Tp, Alloc>;  // 基类

    using Tp_alloc_type =
        typename Base::Tp_alloc_type;  // 正确分配要求类型的分配器类型
    using Alloc_traits =
        std::allocator_traits<Tp_alloc_type>;  // 对应的分配器特性

public:
    // 一系列别名
    using value_type = Tp;                                       // 数值类型
    using pointer = typename Base::pointer;                      // 指针类型
    using const_pointer = typename Alloc_traits::const_pointer;  // 常量指针类型
    using reference = typename Alloc_traits::value_type&;        // 引用类型
    using const_reference =
        const typename Alloc_traits::value_type&;  // 常量引用类型
    using iterator = pointer;                      // 迭代器类型(直接使用指针)
    using const_iterator = const_pointer;          // 常量迭代器
    using reverse_iterator = std::reverse_iterator<pointer>;  // 反向迭代器
    using const_reverse_iterator =
        std::reverse_iterator<const_pointer>;  // 反向常量迭代器
    using size_type = std::size_t;             // 分配的内存尺寸类型
    using difference_type = std::ptrdiff_t;    // 内存地址差值计算类型
    using allocator_type = Alloc;              // 分配器类型

protected:
    // using Base::M_allocate;
    // using Base::M_deallocate

public:
    Vector() = default;

    /**
     * @brief 根据元素个数初始化线性表，调用默认构造函数
     * @param n 需要分配的元素个数
     */
    explicit constexpr Vector(size_type n) : Base(S_check_init_len(n)) {
        M_default_initialize(n);
    }

    /**
     * @brief 析构函数，析构有效范围内的元素
     */
    constexpr ~Vector() noexcept {
        std::destroy(this->M_start, this->M_finish);
    }

protected:
    /**
     * @brief 实现根据默认构造函数初始化指定范围的元素
     * @param first 指向需要初始化区域开头的迭代器
     * @param n 需要初始化的元素个数
     * @return 最后一个需要初始化的元素的后一个迭代器
     */
    constexpr iterator uninitialized_default_n(iterator first, size_type n) {
        iterator cur = first;
        try {
            for (; n > 0; --n, (void)++cur) {
                Alloc_traits::construct(this->alloc, std::addressof(cur));
            }
            return cur;
        } catch (...) {
            std::destroy(first, cur);
            throw;
        }
    }

    /**
     * @brief 分配指定数量的元素内存，并根据默认构造函数初始化它们
     * @param n 需要分配内存的元素个数
     */
    constexpr void M_default_initialize(const size_type n) {
        this->M_finish = uninitialized_default_n(this->M_start, n);
    }

    /**
     * @brief 判断初始长度是否可行
     * @param n 需要分配的元素个数
     * @return n，如果n位于可分配元素个数的范围内，否则抛出异常
     * @exception std::length_error 如果长度超出最大可分配数目
     */
    static constexpr size_type S_check_init_len(const size_type n) {
        if (n > S_max_size()) {
            throw std::length_error(
                "cannot create user::Vector than max_size()"
            );
        }
        return n;
    }

    /**
     * @brief 计算当前容器使用的分配器可分配的元素的最大数量
     * @return 当前分配器可分配的最大元素数量的内存
     */
    static constexpr size_type S_max_size() noexcept {
        // 最大可表示的元素数量(根据指针取值范围)
        const size_type diffmax =
            std::numeric_limits<ptrdiff_t>::max() / sizeof(Tp);
        // 分配器最大可分配的元素数量
        const size_type allocmax = Alloc_traits::max_size(Tp_alloc_type{});
        return std::min(diffmax, allocmax);
    }
};
}  // namespace user

#endif  // VECTOR_HPP
