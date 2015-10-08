namespace ext
{
template <class T, class Allocator>
subvector<T,Allocator>::subvector()
    {
    }

template <class T, class Allocator>
subvector<T,Allocator>::subvector(iterator b, iterator e):
    m_range( b, e )
    {
    }

template <class T, class Allocator>
void subvector<T,Allocator>::link(iterator b, iterator e)
    {
    m_range.first = b;
    m_range.second = e;
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::iterator subvector<T,Allocator>::begin() const
    {
    return m_range.first;
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::iterator subvector<T,Allocator>::end() const
    {
    return m_range.second;
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::iterator subvector<T,Allocator>::rbegin() const
    {
    return end();
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::iterator subvector<T,Allocator>::rend() const
    {
    return begin();
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::size_type subvector<T,Allocator>::size() const
    {
    return std::distance( begin(), end() );
    }

template <class T, class Allocator>
bool subvector<T,Allocator>::empty() const
    {
    return end() == begin();
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::reference subvector<T,Allocator>::operator[] (size_type n) const
    {
    iterator it = begin();
    std::advance( it, n );
    return *it;
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::reference subvector<T,Allocator>::at(size_type n) const
    {
    if ( size() <= n )
        throw std::out_of_range();
    return operator [] ( n );
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::reference subvector<T,Allocator>::front() const
    {
    return *begin();
    }

template <class T, class Allocator>
typename subvector<T,Allocator>::reference subvector<T,Allocator>::back() const
    {
    iterator it = end();
    std::advance( it, -1 );
    return *it;
    }

} //namespace ext

