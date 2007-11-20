/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2007
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_UTILITIES_HPP
#define BOOST_INTRUSIVE_DETAIL_UTILITIES_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/pointer_to_other.hpp>
#include <boost/intrusive/detail/parent_from_member.hpp>
#include <boost/intrusive/detail/ebo_functor_holder.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/detail/assert.hpp>
#include <boost/cstdint.hpp>
#include <cstddef>
#include <climits>
#include <iterator>
#include <boost/cstdint.hpp>

namespace boost {
namespace intrusive {
namespace detail {

template <class T>
struct internal_member_value_traits
{
   template <class U> static detail::one test(...);
   template <class U> static detail::two test(typename U::member_value_traits* = 0);
   static const bool value = sizeof(test<T>(0)) == sizeof(detail::two);
};

template <class T>
struct internal_base_hook_bool
{
   template<bool Add>
   struct two_or_three {one _[2 + Add];};
   template <class U> static one test(...);
   template <class U> static two_or_three<U::boost_intrusive_tags::is_base_hook>
      test (detail::bool_<U::boost_intrusive_tags::is_base_hook>* = 0);
   static const std::size_t value = sizeof(test<T>(0));
};

template <class T>
struct internal_base_hook_bool_is_true
{
   static const bool value = internal_base_hook_bool<T>::value > sizeof(one)*2;
};

template <class T>
struct external_value_traits_bool
{
   template<bool Add>
   struct two_or_three {one _[2 + Add];};
   template <class U> static one test(...);
   template <class U> static two_or_three<U::external_value_traits>
      test (detail::bool_<U::external_value_traits>* = 0);
   static const std::size_t value = sizeof(test<T>(0));
};

template <class T>
struct external_bucket_traits_bool
{
   template<bool Add>
   struct two_or_three {one _[2 + Add];};
   template <class U> static one test(...);
   template <class U> static two_or_three<U::external_bucket_traits>
      test (detail::bool_<U::external_bucket_traits>* = 0);
   static const std::size_t value = sizeof(test<T>(0));
};

template <class T>
struct external_value_traits_is_true
{
   static const bool value = external_value_traits_bool<T>::value > sizeof(one)*2;
};

template<class Node, class Tag, link_mode_type LinkMode, int>
struct node_holder
   :  public Node
{};

template<class SmartPtr>
struct smart_ptr_type
{
   typedef typename SmartPtr::value_type value_type;
   typedef value_type *pointer;
   static pointer get (const SmartPtr &smartptr)
   {  return smartptr.get();}
};

template<class T>
struct smart_ptr_type<T*>
{
   typedef T value_type;
   typedef value_type *pointer;
   static pointer get (pointer ptr)
   {  return ptr;}
};

//!Overload for smart pointers to avoid ADL problems with get_pointer
template<class Ptr>
inline typename smart_ptr_type<Ptr>::pointer
get_pointer(const Ptr &ptr)
{  return smart_ptr_type<Ptr>::get(ptr);   }

//This functor compares a stored value
//and the one passed as an argument
template<class ConstReference>
class equal_to_value
{
   ConstReference t_;

   public:
   equal_to_value(ConstReference t)
      :  t_(t)
   {}

   bool operator()(ConstReference t)const
   {  return t_ == t;   }
};

class null_disposer
{
   public:
   template <class Pointer>
   void operator()(Pointer)
   {}
};

template<class NodeAlgorithms>
class init_disposer
{
   typedef typename NodeAlgorithms::node_ptr node_ptr;

   public:
   void operator()(node_ptr p)
   {  NodeAlgorithms::init(p);   }
};

template<bool ConstantSize, class SizeType>
struct size_holder
{
   static const bool constant_time_size = ConstantSize;
   typedef SizeType  size_type;

   SizeType get_size() const
   {  return size_;  }

   void set_size(SizeType size)
   {  size_ = size; }

   void decrement()
   {  --size_; }

   void increment()
   {  ++size_; }

   SizeType size_;
};

template<class SizeType>
struct size_holder<false, SizeType>
{
   static const bool constant_time_size = false;
   typedef SizeType  size_type;

   size_type get_size() const
   {  return 0;  }

   void set_size(size_type)
   {}

   void decrement()
   {}

   void increment()
   {}
};

template<class KeyValueCompare, class Container>
struct key_nodeptr_comp
   :  private detail::ebo_functor_holder<KeyValueCompare>
{
   typedef typename Container::real_value_traits         real_value_traits;
   typedef typename real_value_traits::node_ptr          node_ptr;
   typedef detail::ebo_functor_holder<KeyValueCompare>   base_t;
   key_nodeptr_comp(KeyValueCompare kcomp, const Container *cont)
      :  base_t(kcomp), cont_(cont)
   {}

   template<class KeyType>
   bool operator()(node_ptr node, const KeyType &key) const
   {  return base_t::get()(*cont_->get_real_value_traits().to_value_ptr(node), key); }

   template<class KeyType>
   bool operator()(const KeyType &key, node_ptr node) const
   {  return base_t::get()(key, *cont_->get_real_value_traits().to_value_ptr(node)); }

   bool operator()(node_ptr node1, node_ptr node2) const
   {
      return base_t::get()
         ( *cont_->get_real_value_traits().to_value_ptr(node1)
         , *cont_->get_real_value_traits().to_value_ptr(node2)
         ); 
   }

   const Container *cont_;
};

template<class F, class Container>
struct node_cloner
   :  private detail::ebo_functor_holder<F>
{
   typedef typename Container::real_value_traits         real_value_traits;
   typedef typename Container::node_algorithms           node_algorithms;
   typedef typename real_value_traits::value_type        value_type;
   typedef typename real_value_traits::pointer           pointer;
   typedef typename real_value_traits::node_traits::node node;
   typedef typename real_value_traits::node_ptr          node_ptr;
   typedef typename real_value_traits::const_node_ptr    const_node_ptr;
   typedef detail::ebo_functor_holder<F>                 base_t;
   enum { safemode_or_autounlink  = 
            (int)real_value_traits::link_mode == (int)auto_unlink   ||
            (int)real_value_traits::link_mode == (int)safe_link     };

   node_cloner(F f, const Container *cont)
      :  base_t(f), cont_(cont)
   {}
   
   node_ptr operator()(node_ptr p)
   {
      node_ptr n = cont_->get_real_value_traits().to_node_ptr
         (*base_t::get()(*cont_->get_real_value_traits().to_value_ptr(p)));
      //Cloned node must be in default mode if the linking mode requires it
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(node_algorithms::unique(n));
      return n;
   }

   node_ptr operator()(const node &to_clone)
   {
      const value_type &v =
         *cont_->get_real_value_traits().to_value_ptr(const_node_ptr(&to_clone));
      node_ptr n = cont_->get_real_value_traits().to_node_ptr(*base_t::get()(v));
      //Cloned node must be in default mode if the linking mode requires it
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(node_algorithms::unique(n));
      return n;
   }

   const Container *cont_;
};

template<class F, class Container>
struct node_disposer
   :  private detail::ebo_functor_holder<F>
{
   typedef typename Container::real_value_traits   real_value_traits;
   typedef typename real_value_traits::node_ptr    node_ptr;
   typedef detail::ebo_functor_holder<F>           base_t;
   typedef typename Container::node_algorithms     node_algorithms;
   enum { safemode_or_autounlink  = 
            (int)real_value_traits::link_mode == (int)auto_unlink   ||
            (int)real_value_traits::link_mode == (int)safe_link     };

   node_disposer(F f, const Container *cont)
      :  base_t(f), cont_(cont)
   {}

   void operator()(node_ptr p)
   {
      if(safemode_or_autounlink)
         node_algorithms::init(p);
      base_t::get()(cont_->get_real_value_traits().to_value_ptr(p));
   }
   const Container *cont_;
};

struct dummy_constptr
{
   dummy_constptr(const void *)
   {}

   const void *get_ptr() const
   {  return 0;  }
};

template<class VoidPointer>
struct constptr
{
   typedef typename boost::pointer_to_other
      <VoidPointer, const void>::type ConstVoidPtr;

   constptr(const void *ptr)
      :  const_void_ptr_(ptr)
   {}

   const void *get_ptr() const
   {  return detail::get_pointer(const_void_ptr_);  }

   ConstVoidPtr const_void_ptr_;
};

template <class VoidPointer, bool store_ptr>
struct select_constptr
{
   typedef typename detail::if_c
      < store_ptr
      , constptr<VoidPointer>
      , dummy_constptr
      >::type type;
};

template <class Container>
struct store_cont_ptr_on_it
{
   typedef typename Container::value_traits value_traits;
   static const bool value = 
      !detail::is_empty_class<value_traits>::value
   || detail::external_value_traits_is_true<value_traits>::value
   ;
};

template<class T, bool Add>
struct add_const_if_c
{
   typedef typename detail::if_c
      < Add
      , typename detail::add_const<T>::type
      , T
      >::type type;
};

template<class Container, bool IsConst>
struct node_to_value
   :  public detail::select_constptr
      < typename boost::pointer_to_other
            <typename Container::pointer, void>::type
      , detail::store_cont_ptr_on_it<Container>::value
      >::type
{
   static const bool store_container_ptr = 
      detail::store_cont_ptr_on_it<Container>::value;

   typedef typename Container::real_value_traits         real_value_traits;
   typedef typename real_value_traits::value_type        value_type;
   typedef typename detail::select_constptr
      < typename boost::pointer_to_other
         <typename Container::pointer, void>::type
      , store_container_ptr >::type                      Base;
   typedef typename real_value_traits::node_traits::node node;
   typedef typename detail::add_const_if_c
         <value_type, IsConst>::type                  vtype;
   typedef typename detail::add_const_if_c
         <node, IsConst>::type                        ntype;
   typedef typename boost::pointer_to_other
      <typename Container::pointer, ntype>::type      npointer;

   node_to_value(const Container *cont)
      :  Base(cont)
   {}

   typedef vtype &                                 result_type;
   typedef ntype &                                 first_argument_type;

   const Container *get_container() const
   {
      if(store_container_ptr)
         return static_cast<const Container*>(Base::get_ptr());
      else
         return 0;
   }

   const real_value_traits *get_real_value_traits() const
   {
      if(store_container_ptr)
         return &this->get_container()->get_real_value_traits();
      else
         return 0;
   }

   result_type operator()(first_argument_type arg) const
   {  return *(this->get_real_value_traits()->to_value_ptr(npointer(&arg))); }
};

template <link_mode_type LinkMode>
struct link_dispatch
{};

template<class Container>
void destructor_impl(Container &cont, detail::link_dispatch<safe_link>)
{  (void)cont; BOOST_INTRUSIVE_SAFE_HOOK_DESTRUCTOR_ASSERT(!cont.is_linked());  }

template<class Container>
void destructor_impl(Container &cont, detail::link_dispatch<auto_unlink>)
{  cont.unlink();  }

template<class Container>
void destructor_impl(Container &, detail::link_dispatch<normal_link>)
{}

template<class T, class NodeTraits, link_mode_type LinkMode, class Tag, int HookType>
struct base_hook_traits
{
   public:
   typedef detail::node_holder
      <typename NodeTraits::node, Tag, LinkMode, HookType>           node_holder;
   typedef NodeTraits                                                node_traits;
   typedef T                                                         value_type;
   typedef typename node_traits::node_ptr                            node_ptr;
   typedef typename node_traits::const_node_ptr                      const_node_ptr;
   typedef typename boost::pointer_to_other<node_ptr, T>::type       pointer;
   typedef typename boost::pointer_to_other<node_ptr, const T>::type const_pointer;
   typedef typename std::iterator_traits<pointer>::reference         reference;
   typedef typename std::iterator_traits<const_pointer>::reference   const_reference;
   static const link_mode_type link_mode = LinkMode;

   static node_ptr to_node_ptr(reference value)
   { return static_cast<node_holder*>(&value); }

   static const_node_ptr to_node_ptr(const_reference value)
   {  return static_cast<const node_holder*>(&value);  }

   static pointer to_value_ptr(node_ptr n) 
   {  return static_cast<T*>(static_cast<node_holder*>(&*n));   }

   static const_pointer to_value_ptr(const_node_ptr n)
   {  return static_cast<const T*>(static_cast<const node_holder*>(&*n));   }
};

template<class T, class Hook, Hook T::* P>
struct member_hook_traits
{
   public:
   typedef Hook                                                      hook_type;
   typedef typename hook_type::boost_intrusive_tags::node_traits     node_traits;
   typedef typename node_traits::node                                node;
   typedef T                                                         value_type;
   typedef typename node_traits::node_ptr                            node_ptr;
   typedef typename node_traits::const_node_ptr                      const_node_ptr;
   typedef typename boost::pointer_to_other<node_ptr, T>::type       pointer;
   typedef typename boost::pointer_to_other<node_ptr, const T>::type const_pointer;
   typedef typename std::iterator_traits<pointer>::reference         reference;
   typedef typename std::iterator_traits<const_pointer>::reference   const_reference;
   static const link_mode_type link_mode = Hook::boost_intrusive_tags::link_mode;

   static node_ptr to_node_ptr(reference value)
   {
      return reinterpret_cast<node*>(&(value.*P));
   }

   static const_node_ptr to_node_ptr(const_reference value)
   {
      return static_cast<const node*>(&(value.*P));
   }

   static pointer to_value_ptr(node_ptr n)
   {
      return detail::parent_from_member<T, Hook>
         (static_cast<Hook*>(detail::get_pointer(n)), P);
   }

   static const_pointer to_value_ptr(const_node_ptr n)
   {
      return detail::parent_from_member<T, Hook>
         (static_cast<const Hook*>(detail::get_pointer(n)), P);
   }
};

//This function uses binary search to discover the
//highest set bit of the integer
inline std::size_t floor_log2 (std::size_t x)
{
   const std::size_t Bits = sizeof(std::size_t)*CHAR_BIT;
   const bool Size_t_Bits_Power_2= !(Bits & (Bits-1));
   BOOST_STATIC_ASSERT(Size_t_Bits_Power_2);

   std::size_t n = x;
   std::size_t log2 = 0;
   
   for(std::size_t shift = Bits >> 1; shift; shift >>= 1){
      std::size_t tmp = n >> shift;
      if (tmp)
   	   log2 += shift, n = tmp;
   }

   return log2;
}

inline float fast_log2 (float val)
{
   boost::uint32_t * const exp_ptr = reinterpret_cast <boost::uint32_t *>(&val);
   boost::uint32_t x = *exp_ptr;
   const int log_2 = (int)(((x >> 23) & 255) - 128);
   x &= ~(255 << 23);
   x += 127 << 23;
   *exp_ptr = x;

   val = ((-1.0f/3) * val + 2) * val - 2.0f/3;

   return (val + log_2);
}

inline std::size_t ceil_log2 (std::size_t x)
{
   return ((x & (x-1))!= 0) + floor_log2(x);
}

template<std::size_t N>
struct sqrt2_pow_max;

template<>
struct sqrt2_pow_max<32>
{
   static const boost::uint32_t value = 0xb504f334;
   static const std::size_t pow   = 31;
};

#ifdef BOOST_HAS_LONG_LONG

template<>
struct sqrt2_pow_max<64>
{
   static const boost::uint64_t value = 0xb504f333f9de6484ull;
   static const std::size_t pow   = 63;
};

#endif

// Returns floor(pow(sqrt(2), x * 2 + 1)).
// Defined for X from 0 up to the number of bits in size_t minus 1.
inline std::size_t sqrt2_pow_2xplus1 (std::size_t x)
{
   const std::size_t value = (std::size_t)sqrt2_pow_max<sizeof(std::size_t)*CHAR_BIT>::value;
   const std::size_t pow   = (std::size_t)sqrt2_pow_max<sizeof(std::size_t)*CHAR_BIT>::pow;
   return (value >> (pow - x)) + 1;
}

} //namespace detail
} //namespace intrusive 
} //namespace boost 

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_DETAIL_UTILITIES_HPP
