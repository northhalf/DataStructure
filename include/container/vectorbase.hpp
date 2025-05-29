/**
 * @file vectorbase.hpp
 * @brief 实现VectorBase类，实现基础的内存分配功能
 */

#ifndef VECTORBASE_HPP
#define VECTORBASE_HPP
#include <concepts>
#include <memory>
#include <userconcept/myconcept.hpp>

/**
 * @struct Vector类的基类，负责内存的分配和释放工作(不执行构造和析构)
 * @tparam Tp 对象类型
 * @tparam Alloc 分配器类型
 */
namespace user {

template <typename Tp, IsAllocator Alloc>
struct VectorBase {
    // 将内存分配器重绑定类型，保证分配器分配的数值类型与Tp相同
    using Tp_alloc_type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Tp>;
    // 获取类型对应的指针类型
    using pointer = typename std::allocator_traits<Alloc>::pointer;
    // 获取分配器特性类型
    using Tr = std::allocator_traits<Tp_alloc_type>;

    Tp_alloc_type alloc;

    pointer M_start;           // 指向开始内存的指针
    pointer M_finish;          // 指向最后一个有效数据的指针
    pointer M_end_of_shorage;  // 指向内存最末尾的指针(不可对其访问)

    /**
     * @brief 实现内存的浅复制
     * @param vb 原对象
     */
    constexpr void M_copy_data(VectorBase const& vb) noexcept {
        M_start = vb.M_start;
        M_finish = vb.M_finish;
        M_end_of_shorage = vb.M_end_of_shorage;
    }

    /**
     * @brief 实现两个VectorBase对象的指向内存的指针的交换
     * @param vb 原对象
     */
    constexpr void M_swap_data(VectorBase& vb) noexcept {
        // 不使用std::swap是为了防止类型信息的丢失，使得编译器难以优化
        VectorBase tmp;
        tmp.M_copy_data(*this);
        M_copy_data(vb);
        vb.M_copy_data(tmp);
    }

    VectorBase() = default;


    /**
     * @brief 分配指定元素个数的内存
     * @param n 需要分配的元素个数
     * @return 指向分配内存起始地址的指针
     */
    constexpr pointer M_allocate(size_t n) {
        // 当需要分配的元素个数不为0的时候分配内存
        return n != 0 ? Tr::allocate(alloc, n) : pointer();
    }

    /**
     * @brief 实现内存的释放
     * @param p 指向需要释放的内存的首地址的指针
     * @param n 需要释放的对象个数
     */
    constexpr void M_deallocate(pointer p, size_t n) {
        Tr::deallocate(alloc,p,n);
    }

    /**
     * @brief 实现内存的分配，并对三个内存指针进行赋值
     * @param n 分配的元素个数
     */
    constexpr void M_create_storage(size_t n) {
        // 指向分配内存的首地址
        this->M_start = this->Tr::allocate(alloc, n);
        // 指向最后的有效地址(初始无值，所以和首地址一致)
        this->M_finish = this->M_start;
        // 指向分配的内存的末地址
        this->M_end_of_shorage = this->M_start + n;
    }
};

}  // namespace user

#endif  // VECTORBASE_HPP
