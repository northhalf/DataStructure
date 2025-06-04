
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
     * @brief 当分配器不是总相同的时候移动其值
     * @param rv 右值user::Vector
     */
    constexpr Vector(Vector&& rv, std::false_type) {
        if (!rv.empty()) {
            this->M_create_storage(rv.size());
            this->M_finish =
                std::uninitialized_move(rv.begin(), rv.end(), this->begin());
            // 将原来的对象清空
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
        M_range_initialize(
            l.begin(), l.end(), std::random_access_iterator_tag{}
        );
    }

    /**
     * @brief 实现根据迭代器范围进行构造
     * @tparam InputIterator 迭代器至少为输入迭代器
     * @param first 指向第一个元素的迭代器
     * @param last 指向最后一个元素的迭代器
     */
    template <std::input_iterator InputIterator>
    constexpr Vector(InputIterator first, InputIterator last) {
        using iterator_type =
            typename std::iterator_traits<InputIterator>::iterator_category;
        M_range_initialize(first, last, iterator_type{});
    }

    /**
     * @brief 析构函数，析构有效范围内的元素
     */
    constexpr ~Vector() noexcept {
        std::destroy(this->M_start, this->M_finish);
    }

    constexpr Vector& operator=(const Vector& other) {
        using pocca =
            typename Alloc_traits::propagate_on_container_copy_assignment::type;
        using is_always_equal = typename Alloc_traits::is_always_equal;
        // 如果两个容器是一样的，则直接返回
        if (std::addressof(other) == this) {
            return *this;
        }

        if (pocca::value) {  // 如果需要在传播过程复制赋值
            if (!is_always_equal::value &&
                this->M_get_Tp_allocator() != other.M_get_Tp_allocator()) {
                // 如果分配器间存在差别，并且两个分配器不同，则需要将原来的内存先释放
                // 新分配器无法释放已经存在的内存

                // 提前将内存释放和析构
                this->clear();
                this->M_deallocate(
                    this->M_start, this->M_end_of_shorage - this->M_start
                );
                // 将指针置为空值
                this->M_start = this->M_finish = this->M_end_of_shorage =
                    nullptr;
            }
            // 分配器替换
            this->M_get_Tp_allocator() = other.M_get_Tp_allocator();
        }
        // 获取另一个容器的元素个数
        const size_type other_size = other.size();

        // 分为三种情况考虑
        // 1. 如果另一个容器的元素个数 > 当前容器的最大存储数量
        // 此时需要重新分配当前容器内存为更大值，并复制元素
        if (other_size > this->capacity()) {
            pointer tmp =
                M_allocate_and_copy(other_size, other.begin(), other.end());
            std::destroy(this->M_start, this->M_finish);
            this->M_deallocate(
                this->M_start, this->M_end_of_shorage - this->M_start
            );
            this->M_start = tmp;
            this->M_end_of_shorage = this->M_start + other_size;
        } else if (this->size() >= other_size) {
            // 如果 另一个元素的元素个数 <= 当前容器的元素个数
            // 将另一个容器复制到当前容器，并将多余的元素析构
            std::destroy(
                std::copy(other.begin(), other.end(), this->begin()),
                this->end()
            );
        } else {
            // 如果 当前容器的元素个数 < 另一个容器的元素个数 < 当前容器的容量
            // 将容器有的元素赋值为另一个容器的相应元素，剩下的元素进行初始化复制
            std::copy(
                this->M_start, this->M_start + this->size(), this->M_start
            );
            std::uninitialized_copy(
                other.M_start + this->size(), other.M_finish, this->M_finish
            );
        }
        // 根据另一个容器的元素个数调整容器最后一个元素的位置
        this->M_finish = this->M_start + other_size;
        return *this;
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
     * @brief 获取第一个元素的引用
     * @return 第一个元素的引用
     */
    [[nodiscard]] constexpr reference front() noexcept { return *begin(); }

    /**
     * @brief 获取第一个元素的常量引用
     * @return 第一个元素的常量引用
     */
    [[nodiscard]] constexpr const_reference front() const noexcept {
        return *begin();
    }

    /**
     * @brief 获取最后一个元素的引用
     * @return 最后一个元素的引用
     */
    [[nodiscard]] constexpr reference back() noexcept { return *(end() - 1); }

    /**
     * @brief 获取最后一个元素的常量引用
     * @return 最后一个元素的常量引用
     */
    [[nodiscard]] constexpr const_reference back() const noexcept {
        return *(end() - 1);
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

    [[nodiscard]] constexpr size_type capacity() const noexcept {
        return static_cast<size_type>(this->M_end_of_shorage - this->M_start);
    }

    /**
     * @brief 清除容器内的所有元素
     */
    constexpr void clear() noexcept { M_erase_at_end(this->M_start); }

    /**
     * @brief 将新元素插入到容器末尾，传入元素的构造函数所需的参数
     * @tparam Args 模板参数包
     * @param args 函数参数包
     * @return 新添加元素后的最后一个元素的引用
     */
    template <typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (this->M_finish != this->M_end_of_shorage) {
            Alloc_traits::construct(
                this->alloc, this->M_finish, std::forward<Args>(args)...
            );
            ++this->M_finish;
        } else {
            this->M_realloc_insert(end(), std::forward<Args>(args)...);
        }
        return back();
    }

    /**
     * @brief 获取当前容器的理论最大容量
     * @return 当前容器理论容量上限
     */
    [[nodiscard]] constexpr size_type max_size() const noexcept {
        return S_max_size();
    }

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
        if (this->M_finish > pos) {
            std::destroy(pos, this->M_finish);
            this->M_finish = pos;
        }
    }
    /**
     * @brief 实现范围内的初始化，并实现了内存的分配
     * @note 此函数要求迭代器至少为前向迭代器
     * @tparam Iterator 至少为迭代器
     * @param first 指向开始范围的迭代器
     * @param last 指向结束范围的迭代器
     */
    template <std::forward_iterator Iterator>
    constexpr void M_range_initialize(
        Iterator first, Iterator last, std::forward_iterator_tag
    ) {
        const size_type n = std::distance(first, last);
        this->M_start = this->M_allocate(S_check_init_len(n));
        this->M_end_of_shorage = this->M_start + n;
        this->M_finish = std::uninitialized_copy(first, last, this->M_start);
    }

    /**
     * @brief 实现传入输入迭代器的初始化操作(输入迭代器只可访问一次)
     * @tparam Iterator 输入迭代器
     * @param first 指向第一个元素的迭代器
     * @param last 指向最后一个元素的后一个元素的迭代器
     */
    template <std::input_iterator Iterator>
    constexpr void M_range_initialize(
        Iterator first, Iterator last, std::input_iterator_tag
    ) {
        try {
            // 依次将每个解引用传入构造函数
            for (; first != last; ++first) {
                this->emplace_back(*first);
            }
        } catch (...) {
            // 如果捕获异常则清除所有已经构造的函数
            this->clear();
            throw;
        }
    }

    /**
     * @brief 实现分配内存并复制元素
     * @tparam ForwardIterator 至少为前向迭代器类型
     * @param n 需要分配的元素个数
     * @param first 指向第一个元素的迭代器
     * @param last 指向最后一个元素的迭代器
     * @return 指向新的内存区域首地址的指针
     */
    template <std::forward_iterator ForwardIterator>
    constexpr pointer M_allocate_and_copy(
        size_type n, ForwardIterator first, ForwardIterator last
    ) {
        pointer result = this->M_allocate(n);
        try {
            std::uninitialized_copy(first, last, result);
            return result;
        } catch (...) {
            this->M_deallocate(result, n);
            throw;
        }
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
     * @brief 实现指定位置插入对象，并重新内存分配
     * @tparam Args 模板参数包，需要构造的对象的构造函数参数类型
     * @param position 插入元素位置
     * @param args 函数参数包，构造对象的构造函数参数
     */
    template <typename... Args>
    constexpr void M_realloc_insert(iterator position, Args&&... args) {
        // 获取此时新分配内存存储的元素总个数
        const size_type len =
            M_check_len(static_cast<size_type>(1), "Vector::M_realloc_insert");
        pointer old_start = this->M_start;
        pointer old_finish = this->M_finish;
        // 这个位置前有多少元素
        const size_type elems_before = position - begin();
        pointer new_start = this->M_allocate(len);
        pointer new_finish = new_start;
        try {
            // 在插入位置构造新元素
            Alloc_traits::construct(
                this->alloc, new_start + elems_before,
                std::forward<Args>(args)...
            );
            // 根据是否可移动采取移动或者构造函数
            if constexpr (std::is_move_constructible_v<Tp>) {
                // 将原来内存区域的值移动到新区域
                new_finish =
                    std::uninitialized_move(old_start, position, new_start);
                ++new_finish;
                new_finish =
                    std::uninitialized_move(position, old_finish, new_finish);
            } else if constexpr (std::is_copy_constructible_v<Tp>) {
                // 如果不可移动但是可以复制
                new_finish =
                    std::uninitialized_copy(old_start, position, new_start);
                ++new_finish;
                new_finish =
                    std::uninitialized_copy(position, old_finish, new_finish);
            } else {
                // 既不可移动也不可复制，则抛出异常
                throw std::runtime_error(
                    "M_realloc_insert: The type of the value is unable to copy "
                    "or move"
                );
            }
        } catch (...) {  // 如果移动对象出现了错误
            // 析构当前已经插入的对象
            std::destroy(new_start, new_finish);
            // 将新分配的内存释放
            this->M_deallocate(new_start, len);
            throw;
        }
        // 释放原来分配的内存区域
        this->M_deallocate(old_start, this->M_end_of_shorage - old_start);
        // 重设三个指针
        this->M_start = new_start;
        this->M_finish = new_finish;
        this->M_end_of_shorage = new_start + len;
    }

    /**
     * @brief 用于检测需要新分配的元素数量是否处于正常范围内
     * @param n 需要新分配的元素个数
     * @param s 调用函数名以及其他信息
     * @throw std::length_error 最大分配元素个数不足以分配这个元素
     * @return 应当分配内存的元素个数
     */
    constexpr size_type M_check_len(size_type n, const char* s) const {
        // 如果剩下的元素空间不足，则抛出异常
        if (max_size() - size() < n) {
            throw std::length_error(s);
        }
        // 计算需要分配的内存空间，如果两倍空间不够，则len+n分配内存空间
        const size_type len = size() + std::max(size(), n);
        // 如果出现了溢出等情况则分配最大空间
        return (len < size() || len > max_size()) ? max_size() : len;
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
 * @brief 重载流插入运算符实现输出容器内的元素，默认5个一行
 * @tparam Tp 可插入类型
 * @tparam num_ele_one_line 一行输出的元素个数，默认为5
 * @param os 输出流
 * @param vec 要输出的向量
 * @return os的引用
 */
template <IsInsertable Tp, size_t num_ele_one_line = 5>
std::ostream& operator<<(std::ostream& os, Vector<Tp>& vec) {
    size_t count = 0;
    for (auto& it : vec) {
        os << it << " ";
        count++;
        if (count == num_ele_one_line) {
            os << std::endl;
            count = 0;
        }
    }
    return os;
}
}  // namespace user

#endif  // VECTOR_HPP
