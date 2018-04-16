/**
 * 这个文件用来实现switch string
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-15T15:05:06+08:00
 * @Email:  guang334419520@126.com
 * @Filename: Hash.hpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-15T17:04:35+08:00
 */
#include <string>
typedef std::uint64_t hash_t;

template <class T> struct Hash {};

template <> struct Hash<char*> {
  hash_t operator()(const char* s) const
  {
    hash_t h = 0;
    for ( ; *s; s++)
      h = 5*h + *s;
    return h;
  }
};

template <> struct Hash<const char*> {
  hash_t operator()(const char* s) const
  {
    hash_t h = 0;
    for ( ; *s; s++)
      h = 5*h + *s;
    return h;
  }
};

template <> struct Hash<std::string> {
  hash_t operator()(const std::string& str) const
  {
    hash_t h = 0;
    int len = str.size();
    for (int i = 0; i < len; i++)
      h = 5*h + str.at(i);
    return h;
  }
};

template <class T>
inline hash_t HashFunc(T s)
{
  return Hash<T>()(s);
}


constexpr hash_t HashCompile(const char* str, hash_t last_value = 0)
{
  return *str ? HashCompile(str + 1, last_value*5 + *str) : last_value;
}
