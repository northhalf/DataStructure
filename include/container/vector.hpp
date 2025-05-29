
/**
 * @file vector.hpp
 * @brief 实现自定义的Vector类实现
 */

#ifndef VECTOR_HPP
#define VECTOR_HPP
#include <concepts>
#include <container/vectorbase.hpp>
#include <memory>
#include <userconcept/myconcept.hpp>
namespace user {

template <
    typename Tp, IsAllocator Alloc = std::allocator<Tp>,
    typename = std::enable_if_t<std::is_same_v<Tp, typename Alloc::value_type>>>
class Vector : protected VectorBase<Tp, Alloc> {};

}  // namespace user

#endif  // VECTOR_HPP
