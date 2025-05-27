/**
 *@file poolmemory.hpp
 *@brief 实现基础的内存池类
 */

#ifndef POOLMEMORY_HPP
#define POOLMEMORY_HPP
#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace user {

/**
 * @class PoolMemory
 * @brief 实现简单的内存池功能，每个内存池存放1024个元素
 * @warning 不可用于分配连续内存，内存池中各元素都是分开的
 */
template <typename T>
class PoolMemory {
    /**
     * @struct Block
     * @brief 内存块结构体，标记内存块是否空闲以及上一空闲内存块和
     * 下一空闲内存块的地址
     */
    struct Block {
        bool free;               // 该内存块是否空闲
        Block *next_free_block;  // 下一空闲内存块
        Block *prev_free_block;  // 上一空闲内存块
    };
    /**
     * @struct Page
     * @brief
     * 内存页结构体，第一个空闲内存块地址和下一内存页地址
     */
    struct Page {
        Block *first_free_block;  // 内存页中第一个空闲的内存块
        Page *next_page;          // 下一内存页地址
    };

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
    // 两个内存池总是不同的
    using is_always_equal = std::false_type;

    // 内存池不允许复制
    PoolMemory(const PoolMemory &) = delete;
    PoolMemory &operator=(const PoolMemory &) = delete;

    PoolMemory() : first_page(nullptr) {}
    ~PoolMemory() {
        // 释放每个内存池
        for (Page *page = first_page; page != nullptr;) {
            Page *temp = page;
            page = page->next_page;
            ::operator delete(temp);
        }
    }

    /**
     * @brief 分配一个空闲的内存地址
     * @param n 本应该为分配的连续元素个数，但此内存池中无用，不可大于1
     * @return 空闲的内存地址
     * @warning 分配的元素个数不可超过1
     */
    void *allocate(size_type n) {
        // 不允许超过一个元素的分配
        if (n > 1) {
            throw std::out_of_range(
                "PoolMemory::allocate: n cannot be greater than one"
            );
        }
        return allocate();
    }
    /**
     * @brief 为一个元素分配内存地址
     * @return 空闲的内存地址
     */
    void *allocate() {
        // 首先查找空闲内存页
        Page *free_page = FindFreePage();
        // 如果没找到空内存页则分配新的内存页
        if (free_page == nullptr) {
            // 判断是否此时一个内存页都没有
            if (first_page == nullptr) {
                // 分配第一个内存页，并将此内存页作为空闲内存页
                free_page = first_page = AllocNewPage();
            } else {
                // 此时有内存页
                // 查找到最后一个内存页
                Page *last_page = get_last_page();
                // 将新内存页作为最后一个内存页，并将原内存页与其进行连接
                last_page->next_page = AllocNewPage();
                free_page = last_page = last_page->next_page;
            }
        }
        // 获取第一个空闲内存块地址
        Block *free_block = free_page->first_free_block;

        // 将空闲内存块设为非空闲，并连接前后链表
        free_block->free = false;
        // 如果后空闲内存块存在，将其前内存块地址置为nullptr
        if (free_block->next_free_block != nullptr) {
            free_block->next_free_block->prev_free_block = nullptr;
        }
        // 将此空闲内存块的下一内存块置为第一空闲内存块
        free_page->first_free_block = free_block->next_free_block;
        // 将这个内存块移出链表
        free_block->next_free_block = free_block->prev_free_block = nullptr;

        // 将空闲内存页的第一个空内存块相应地址返回
        return reinterpret_cast<std::byte *>(free_block) + block_info_size;
    }

    /**
     * @brief  释放指定地址的内存
     * @param p 需要释放的内存地址
     * @param n 需要释放的元素个数，实际只可释放一个地址的内存，所以n不可超过1
     * @note 本质是释放一个指定地址的内存
     * @warning n不可超过1
     */
    void deallocate(void *p, size_type n) {
        if (n > 1) {
            throw std::out_of_range(
                "PoolMemory::deallocate: n cannot be greater than one"
            );
        }
        deallocate(p);
    }
    /**
     * @brief 释放指定地址的内存
     * @param p 需要释放的内存地址
     */
    void deallocate(void *p) {
        // 将指针前移到内存块起始地址
        auto block = reinterpret_cast<Block *>(
            static_cast<std::byte *>(p) - block_info_size
        );
        // 将此内存块设置为空闲内存块
        block->free = true;
        // 查找这个内存块在哪一个内存页中
        Page *page = first_page;
        while (page != nullptr) {
            // 如果找到内存块在某一内存页范围内则跳过
            if (reinterpret_cast<std::byte *>(page) <
                    reinterpret_cast<std::byte *>(block) and
                reinterpret_cast<std::byte *>(block) <
                    reinterpret_cast<std::byte *>(page) + page_size) {
                break;
            }
            page = page->next_page;
        }
        // 如果在任何内存页中则抛出错误
        if (page == nullptr) {
            throw std::invalid_argument(
                "PoolMemory::deallocate: invalid pointer"
            );
        }
        // 如果此内存页没有空闲内存块
        if (page->first_free_block == nullptr) {
            // 将此内存块设为第一个内存块
            page->first_free_block = block;
            block->prev_free_block = block->next_free_block = nullptr;
            return;
        }

        /* 存在空闲内存块 */
        // 查找相邻内存块
        Block *adjacent_block = page->first_free_block;
        // 查找内存块直到到了该内存块的后面
        while (adjacent_block < block and
               adjacent_block->next_free_block != nullptr) {
            // 如果刚好是前一内存块则终止循环
            if (adjacent_block < block and
                block < adjacent_block + block_size) {
                break;
            }
            adjacent_block = adjacent_block->next_free_block;
        }
        // 将此内存块连接到空闲内存块链表中
        // 此时有两种可能，相邻内存块可能是在此之前，也可能在此之后
        if (adjacent_block < block) {
            // 如果相邻内存块在该内存块之前
            block->prev_free_block = adjacent_block;
            block->next_free_block = adjacent_block->next_free_block;

            adjacent_block->next_free_block = block;
            if (block->next_free_block != nullptr) {
                block->next_free_block->prev_free_block = block;
            }
        } else {
            // 如果相邻内存块在该内存块之后
            block->next_free_block = adjacent_block;
            block->prev_free_block = adjacent_block->prev_free_block;

            adjacent_block->prev_free_block = block;
            if (block->prev_free_block != nullptr) {
                block->prev_free_block->next_free_block = block;
            }
        }
    }

private:
    /**
     * @brief  获取内存块信息占用的内存大小
     * @return 内存块信息所占用的字节数
     */
    static constexpr size_type get_block_info_size() {
        // 获取目标类型的对齐要求
        constexpr size_type align_value_type = alignof(value_type);
        // 如果目标对齐要求>=32字节，则对齐要求就是内存块信息需要占用的字节数
        if constexpr (align_value_type >= 32) {
            return align_value_type;
        }
        // 如果目标对齐要求==16字节，那么内存块信息需要占用32字节
        if constexpr (align_value_type == 16) {
            return 32;
        }
        // 目标对齐要求<=8字节，内存块信息占用大小就是其本身大小
        return sizeof(Block);
    }
    /**
     * @brief 获取当前内存池的内存页字节大小
     * @return 内存页的字节大小
     */
    static constexpr size_type get_page_size() {
        return page_info_size + block_size * num_ele;
    }
    /**
     * @brief 获取当前内存块的占用大小
     * @return 当前内存块占用大小
     */
    static constexpr size_type get_block_size() {
        constexpr size_type align_value_type = alignof(value_type);
        // 如果目标类型的内存对齐要求>=16字节，则以此字节作为目标对齐要求
        if constexpr (align_value_type >= 32) {
            return align_value_type + sizeof(value_type);
        }
        // 如果对齐要求为16字节，这时候内存块信息需要32字节存放
        if constexpr (align_value_type == 16) {
            return 32 + sizeof(value_type);
        }

        // 例如: 如果目标元素类型大小为12,则上取到16
        // 实现：位运算，先+7(0b111)，再将低三位置为0(8的低三位为0)
        size_type value_size = (sizeof(value_type) + 0b111) & ~0b111;
        return sizeof(Block) + value_size;
    }
    /**
     * @brief 分配内存给新的内存页
     * @return 指向分配的内存页的指针
     */
    Page *AllocNewPage() {
        // 为新内存页分配对应内存
        alignas(align_bytes) Page *new_page =
            static_cast<Page *>(::operator new(page_size));

        // 确定第一个内存块的内存地址
        new_page->first_free_block = reinterpret_cast<Block *>(
            reinterpret_cast<std::byte *>(new_page) + page_info_size
        );

        // 依次确定每个内存块的信息
        // 枚举内存块索引
        for (size_t num_block = 0; num_block < num_ele; num_block++) {
            // 计算该索引对应内存块的地址
            auto block = reinterpret_cast<Block *>(
                reinterpret_cast<std::byte *>(new_page->first_free_block) +
                block_size * num_block
            );

            // 内存块设置为空闲
            block->free = true;
            // 如果不是最后一个内存块则设置下一内存块为其空闲内存块
            if (num_block != num_ele - 1) {
                block->next_free_block = reinterpret_cast<Block *>(
                    reinterpret_cast<std::byte *>(block) + block_size
                );
            } else {
                // 最后一个内存块没有下一空闲内存
                block->next_free_block = nullptr;
            }

            // 如果不是第一个内存块则设置上一内存块为其空闲内存块
            if (num_block != 0) {
                block->prev_free_block = reinterpret_cast<Block *>(
                    reinterpret_cast<std::byte *>(block) - block_size
                );
            } else {
                // 第一个内存块没有上一空闲内存块
                block->prev_free_block = nullptr;
            }
        }
        // 将下一内存页的地址设置为nullptr
        new_page->next_page = nullptr;
        return new_page;
    }

    /**
     * @brief 获取该内存池的最后一个内存页
     * @return 最后一个内存页的地址
     */
    Page *get_last_page() {
        Page *temp = first_page;
        while (temp->next_page != nullptr) {
            temp = temp->next_page;
        }
        return temp;
    }

    /**
     * @brief 查询内存池的空闲内存块
     * @return 如果存在空闲内存页，则返回其地址;否则返回nullptr
     */
    Page *FindFreePage() {
        // 从第一个内存页开始遍历内存页
        for (Page *temp = first_page; temp != nullptr; temp = temp->next_page) {
            // 如果查找到空闲内存页则将地址返回
            if (temp->first_free_block != nullptr) {
                return temp;
            }
        }
        // 没有查找到内存页则返回空指针
        return nullptr;
    }

    // 获取此时的内存对齐要求
    constexpr static size_type align_bytes =
        alignof(value_type) >= 16 ? alignof(value_type) : 8;
    // 内存页的信息大小
    constexpr static size_type page_info_size =
        alignof(value_type) >= 32 ? alignof(value_type) : 16;
    // 获取此时的内存块大小
    constexpr static size_type block_size = get_block_size();
    // 获取此时的内存块信息的占用内存
    constexpr static size_type block_info_size = get_block_info_size();
    // 记录内存页的大小
    constexpr static size_type page_size = get_page_size();
    // 存储目标类型对象的个数
    constexpr static size_type num_ele = 1024;

    Page *first_page;  // 第一个内存页
};

}  // namespace user

#endif  // POOLMEMORY_HPP
