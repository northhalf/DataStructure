/**
 * @file my-poolmemory.hpp
 * @brief user::PoolMemory类
 * @details 定义内存池分配器类
 */

#ifndef MY_POOLMEMORY_HPP
#define MY_POOLMEMORY_HPP
#include <type_traits>
namespace user {

/**
 * @class PoolMemory
 * @brief 实现底层为内存池方式的内存的分配和释放
 * @warning 无法用于替换std::allocator，仅用于小块内存频繁取用的情况
 */
template <typename T>
class PoolMemory {
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
    // 两个内存分配器总是不同的
    using is_always_equal = std::false_type;

    // 内存池不允许复制
    PoolMemory(const PoolMemory&) = delete;
    PoolMemory& operator=(const PoolMemory&) = delete;

    /**
     * @brief 内存池的构造函数，构造一个拥有1000的元素大小的内存页
     */
    PoolMemory();

    /**
     * @brief 内存池析构函数，释放每个内存页
     */
    ~PoolMemory() {
        // 依次释放内存页的内存
        while (begin != nullptr) {
            MemoryPage* temp = begin;
            begin = begin->next;
            operator delete(temp);
        }
    }

    /**
     * @brief 分配内存
     * @note 最大分配个数为1000
     * @param n 分配的内存的对象数量
     */
    value_type* allocate(size_type n);

    /**
     * @brief 释放指定个数的对象内存
     * @warning 最后分配的n个对象此时应当弃用
     * @param n 释放内存的对象数量
     */
    void deallocate(size_type n);

private:
    /**
     * @struct MemoryPage
     * @brief
     * 内存页结构体，拥有该内存页是否存在空闲，尺寸信息，起点和终点地址和下一个内存页地址
     */
    struct MemoryPage {
        MemoryPage* next;  // 下一个内存页的地址

        value_type* curr_block;  // 记录内存页的当前内存块位置
        value_type* begin;       // 内存页的起点地址
        value_type* end;         // 内存页的终点地址
    };

    // 一个内存池拥有存储1000个value_type的内存
    static constexpr size_type page_size = 1000;

    MemoryPage* begin;  // 第一个内存页的地址
    MemoryPage* end;    // 最后一个内存页的地址
};

template <typename T>
inline bool operator==(
    const PoolMemory<T>& lhs, const PoolMemory<T>& rhs
) noexcept {
    return false;
}

// 内存池构造函数
template <typename T>
PoolMemory<T>::PoolMemory() {
    // 创建第一个内存页(内存页的前面内存存储内存页的基本信息)
    begin = end = static_cast<MemoryPage*>(
        ::operator new(sizeof(MemoryPage) + page_size * sizeof(T))
    );

    // 第一个内存页的起点地址
    begin->begin = reinterpret_cast<value_type*>(begin + 0);
    // 第一个内存页的终点地址
    begin->end = reinterpret_cast<value_type*>(begin + 0 + page_size);
    // 此时没有下一个内存页
    begin->next = nullptr;
    // 设置第一个空闲内存块信息
    begin->curr_block = begin->begin;
}

template <typename T>
T* PoolMemory<T>::allocate(size_type n) {
    // 如果需要分配的对象个数大于内存页个数，则报错
    if (n > page_size) {
        throw std::bad_alloc();
    }

    // 如果剩余内存不足，则创建新内存页
    if (static_cast<long unsigned int>(end->end - end->curr_block) < n) {
        // 创建新内存页(内存页的前面内存存储内存页的基本信息)
        MemoryPage* temp = static_cast<MemoryPage*>(
            ::operator new(sizeof(MemoryPage) + page_size * sizeof(T))
        );
        // 内存页的起点地址
        temp->begin = reinterpret_cast<value_type*>(begin + 0);
        // 内存页的终点地址
        temp->end = reinterpret_cast<value_type*>(begin + 0 + page_size);
        // 内存页的首个内存块地址
        temp->curr_block = temp->begin;
        // 此时没有下一个内存页
        temp->next = nullptr;

        // 原来的内存页和这个内存页连接
        end->next = temp;
        end = temp;
    }
    // 分配指定内存，并将内存块指针后移
    value_type* res = end->curr_block;
    end->curr_block += n;

    return res;
}

template <typename T>
void PoolMemory<T>::deallocate(size_type n) {
    // 判断能否直接将内存块指针前移
    if (end->curr_block - end->begin >= n) {
        // 空间能够直接前移
        end->curr_block -= n;
        return;
    }
    // 空间比前移小，那么试图释放当前内存页，并将前一内存页内存块指针前移
    // 先判断是否只有一个内存页
    if (begin == end) {
        // 如果只有一个内存页了，那么只将内存块指针置为最前面
        end->curr_block = end->begin;
        return;
    }
    // 不是只有一个内存页，则释放当前内存页并递归调用deallocate函数
    // 复制当前内存页地址
    MemoryPage* temp = end;
    // 计算还需要前移多少
    size_type remain = n - (end->curr_block - end->begin);
    // 查找end的前一内存页地址
    MemoryPage* cur = begin;
    for (; cur->next != end; cur = cur->next);
    end = cur;

    operator delete(temp);
    deallocate(remain);
}

}  // namespace user

#endif