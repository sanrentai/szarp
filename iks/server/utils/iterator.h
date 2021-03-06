#ifndef __UTILS_ITERATOR_H__
#define __UTILS_ITERATOR_H__

/**
 * This class is using Map::value_type::first_type instead of key_type
 * to work properly with boost::multi_index_container
 */
template<class Map>
class key_iterator
{
public:
    key_iterator( const key_iterator& itr ) : itr(itr.itr) {}
    key_iterator( const typename Map::const_iterator& itr ) : itr(itr) {}
    const typename Map::value_type::first_type* operator->() const { return &itr->first;    }
    const typename Map::value_type::first_type& operator* () const { return itr->first;     } 
    key_iterator&  operator++()    { ++itr; return *this;    }
    key_iterator   operator++(int) { return key_iterator(itr++); }
    bool operator==( const key_iterator& other ) { return itr == other.itr; }
    bool operator!=( const key_iterator& other ) { return itr != other.itr; }
protected:
    typename Map::const_iterator itr;
};

#endif /* __UTILS_ITERATOR_H__ */

