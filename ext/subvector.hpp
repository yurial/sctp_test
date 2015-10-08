#ifndef EXT_SUBVECTOR_HPP
#define EXT_SUBVECTOR_HPP

#include <memory>
#include <stdexcept>
#include <vector>

namespace ext
{

template <class T, class Allocator=std::allocator<T> >
class subvector
    {
    public:
        typedef std::vector<T,Allocator>                base;
        typedef typename base::reference                reference;
        typedef typename base::const_reference          const_reference;
        typedef typename base::iterator                 iterator;
        typedef typename base::const_iterator           const_iterator;
        typedef typename base::size_type                size_type;
        typedef typename base::difference_type          difference_type;
        typedef typename base::value_type               value_type;
        typedef typename base::allocator_type           allocator_type;
        typedef typename base::pointer                  pointer;
        typedef typename base::const_pointer            const_pointer;
        typedef typename base::reverse_iterator         reverse_iterator;
        typedef typename base::const_reverse_iterator   const_reverse_iterator;

    protected:
        std::pair<iterator,iterator>    m_range;

    public:
        inline              subvector();
        inline              subvector(iterator begin, iterator end);
        inline  void        link(iterator begin, iterator end);

        inline  iterator    begin() const;
        inline  iterator    end() const;
        inline  iterator    rbegin() const;
        inline  iterator    rend() const;

        inline  size_type   size() const;
        inline  bool        empty() const;

        inline  reference   operator[] (size_type n) const;
        inline  reference   at(size_type n) const;
        inline  reference   front() const;
        inline  reference   back() const;
    };

} //namespace ext

#include "subvector.inc"

#endif

