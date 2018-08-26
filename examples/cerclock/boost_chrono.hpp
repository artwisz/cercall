#ifndef BOOST_SERIALIZATION_CHRONO_HPP
#define BOOST_SERIALIZATION_CHRONO_HPP

#include <chrono>
#include <boost/config.hpp>

#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

/// serialization for std::chrono::duration
template<class Archive, class Rep, class Period >
inline void serialize(Archive& ar, std::chrono::duration<Rep, Period>& t, const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}

template<class Archive, class Rep, class Period>
inline void save(Archive & ar, std::chrono::duration<Rep, Period> const& t, const unsigned int /* file_version */)
{
    ar << t.count();
}

template<class Archive, class Rep, class Period>
inline void load(Archive& ar, std::chrono::duration<Rep, Period>& t, const unsigned int /* file_version */)
{
    Rep rep;
    ar >> rep;
    t = std::chrono::duration<Rep, Period>(rep);
}

// specialization of serialization traits for std::chrono::duration
template <class Rep, class Period>
struct is_bitwise_serializable<std::chrono::duration<Rep, Period>> : public is_bitwise_serializable<Rep> {};

template <class Rep, class Period>
struct implementation_level<std::chrono::duration<Rep, Period>> : mpl::int_<object_serializable> {};

// treat just like builtin arithmetic types for tracking
template <class Rep, class Period>
struct tracking_level<std::chrono::duration<Rep, Period>> : mpl::int_<track_never> {};

/// serialization for std::chrono::time_point
template<class Archive, class C, class D>
inline void serialize(Archive & ar, std::chrono::time_point<C,D>& t, const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}

template<class Archive, class C, class D>
inline void save(Archive & ar, std::chrono::time_point<C, D> const& t, const unsigned int /* file_version */)
{
    ar << t.time_since_epoch();
}

template<class Archive, class C, class D>
inline void load(Archive & ar, std::chrono::time_point<C,D>& t, const unsigned int /* file_version */)
{
    D dur;
    ar >> dur;
    t = std::chrono::time_point<C,D>(dur);
}

// specialization of serialization traits for std::chrono::time_point
template <class C, class D>
struct is_bitwise_serializable<std::chrono::time_point< C,D >> : public is_bitwise_serializable<D> {};

template <class C, class D>
struct implementation_level<std::chrono::time_point<C,D>>  : mpl::int_<object_serializable> {};

// treat just like builtin arithmetic types for tracking
template <class C, class D>
struct tracking_level<std::chrono::time_point< C,D > > : mpl::int_<track_never> {};

} // serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_CHRONO_HPP
