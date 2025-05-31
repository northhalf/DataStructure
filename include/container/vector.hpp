
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
 * @tparam Tp 要存储的数据类型，不可为const或volatile修饰类型
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
    explicit constexpr Vector(const size_type n) : Base(S_check_init_len(n)) {
        M_default_initialize(n);
    }

    /**
     * @brief 根据传入的值批量初始化
     * @param n 需要的初始元素个数
     * @param value 需要赋的初值
     */
    constexpr Vector(const size_type n, const value_type& value)
        : Base(S_check_init_len(n)) {
        M_fill_initialize(n, value);
    }
    /**
     * @brief 复制构造函数
     * @param other 需要复制的user::Vector
     */
    constexpr Vector(const Vector& other) : Base(other.size()) {
        this->M_finish =
            std::uninitialized_copy(other.begin(), other.end(), this->begin());
    }

private:
    /**
     * @brief 当分配器总相同时直接转移指针
     * @param rv 右值user::Vector
     */
    constexpr Vector(Vector&& rv, std::true_type) noexcept
        : Base(std::move(rv)) {}
    /**
     * @brief 当分配器不是总相同的时候复制其值
     * @param rv 右值user::Vector
     */
    constexpr Vector(Vector&& rv, std::false_type) {
        if (!rv.empty()) {
            this->M_create_storage(rv.size());
            this->M_finish =
                std::uninitialized_move(rv.begin(), rv.end(), this->begin());
            rv.clear();
        }
    }

public:
    /**
     * @brief 移动构造函数
     * @param rv 右值user::Vector
     */
    constexpr Vector(Vector&& rv) noexcept(noexcept(Vector(
        std::declval<Vector&&>(),
        std::declval<typename Alloc_traits::is_always_equal>(
        )  // 判断分配器是否总是相同
    )))
        : Vector(std::move(rv), typename Alloc_traits::is_always_equal{}) {}
    /**
     * @brief 根据初始化列表初始化user::Vector
     * @param l 初始化列表
     */
    constexpr Vector(std::initializer_list<value_type> l) {
        M_range_initialize(l.begin(), l.end());
    }
    /**
     * @brief 析构函数，析构有效范围内的元素
     */
    constexpr ~Vector() noexcept {
        std::destroy(this->M_start, this->M_finish);
    }

    /**
     * @brief 获取第一个元素的迭代器
     * @return 指向第一个元素的可读写迭代器
     */
    [[nodiscard]] constexpr iterator begin() noexcept {
        return static_cast<iterator>(this->M_start);
    }
    /**
     * @brief 获取第一个元素的常量迭代器
     * @return 指向第一个元素的只读迭代器
     */
    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return static_cast<const_iterator>(this->M_start);
    }
    /**
     * @brief 获取指向最后一个元素后面的迭代其
     * @return 指向最后一个元素后面的迭代器
     * @warning 对这个迭代器进行更改的行为是未定义的
     */
    [[nodiscard]] constexpr iterator end() noexcept {
        return static_cast<iterator>(this->M_finish);
    }
    /**
     * @brief 获取最后一个元素后面的只读迭代器
     * @return 获取指向最后一个元素后面的只读迭代器
     */
    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return static_cast<const_iterator>(this->M_finish);
    }
    /**
     * @brief 获取容器此时状态是否为空
     * @return 如果容器为空，返回true，否则返回false
     */
    [[nodiscard]] constexpr bool empty() noexcept { return begin() == end(); }
    /**
     * @brief 获取容器内的元素个数
     * @return user::Vector中包含的元素个数
     * @note 其返回值不应被忽略
     */
    [[nodiscard]] constexpr size_type size() const noexcept {
        return static_cast<size_type>(this->M_finish - this->M_start);
    }

    /**
     * @brief 清除容器内的所有元素
     */
    constexpr void clear() noexcept { M_erase_at_end(this->M_start); }

protected:
    /**
     * @brief 分配指定数量的元素内存，并根据默认构造函数初始化它们
     * @param n 需要分配内存的元素个数
     */
    constexpr void M_default_initialize(const size_type n) {
        this->M_finish =
            std::uninitialized_default_construct_n(this->M_start, n);
    }
    /**
     * @brief 实现清除从指定位置到末尾的所有元素
     * @param pos 开始清除元素的位置
     */
    constexpr void M_erase_at_end(pointer pos) noexcept {
        if (this->M_finish - pos != nullptr) {
            std::destroy(pos, this->M_finish);
            this->M_finish = pos;
        }
    }
    /**
     * @brief 实现范围内的初始化
     * @tparam Iterator 迭代器类型
     * @param first 指向开始范围的迭代器
     * @param last 指向结束范围的迭代器
     */
    template <std::input_iterator Iterator>
    constexpr void M_range_initialize(Iterator first, Iterator last) {
        const size_type n = std::distance(first, last);
        this->M_start = this->M_allocate(S_check_init_len(n));
        this->M_end_of_shorage = this->M_start + n;
        this->M_finish = std::uninitialized_copy(first, last, this->M_start);
    }
    /**
     * @brief 根据传入的值进行初始化
     * @param n 需要初始化的元素个数
     * @param value 初始化的数值
     */
    constexpr void M_fill_initialize(
        const size_type n, const value_type& value
    ) {
        this->M_finish = std::uninitialized_fill_n(this->M_start, n, value);
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
/**
 * @brief 重载流插入运算符实现输出容器内的元素
 * @tparam Tp 可插入类型
 * @param os 输出流
 * @param vec 要输出的向量
 * @return os的引用
 */
template <IsInsertable Tp>
std::ostream& operator<<(std::ostream& os, Vector<Tp>& vec) {
    size_t count = 0;
    for (auto& it : vec) {
        os << it << " ";
        if (count == 5) {
            os << std::endl;
            count = 0;
        }
    }
    return os;
}
}  // namespace user

#endif  // VECTOR_HPP
