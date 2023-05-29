#ifndef __ZERO_MYENDIAN_H__
#define __ZERO_MYENDIAN_H__

#include <cstdint>
#include <type_traits>
#include <byteswap.h>
#include <stdint.h>
/// 不包含该头文件，出现大小端乱识别的问题
#include <endian.h>

#define ZERO_LITTLE_ENDIAN 1
#define ZERO_BIG_ENDIAN 2

namespace zero {

/**
 * @brief 8字节类型的字节序转化
 * 
 * @tparam T 
 * @param value 
 * @return std::enable_if<bool,T>::type ,当bool条件为true时，返回T类型
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t),T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 4字节类型的字节序转化
 * 
 * @tparam T 
 * @param value 
 * @return std::enable_if<sizeof(T) == sizeof(uint32_t),T>::type 
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t),T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 2字节类型的字节序转化
 * 
 * @tparam T 
 * @param value 
 * @return std::enable_if<sizeof(T) == sizeof(uint16_t),T>::type 
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t),T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define ZERO_BYTE_ORDER ZERO_BIG_ENDIAN
#else
#define ZERO_BYTE_ORDER ZERO_LITTLE_ENDIAN
#endif

#if ZERO_BYTE_ORDER == ZERO_BIG_ENDIAN

/**
 * @brief 只在小端机器上执行byteswap,在大端机器上什么都不做
 * 
 * @tparam T 
 * @param t 
 * @return T 
 */
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

/**
 * @brief 只在大端机器上执行byteswap,在小端机器上什么都不做
 * 
 * @tparam T 
 * @param t 
 * @return T 
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}


#else

/**
 * @brief 网络字节序是大端的，在小端机器上执行byteswap转换为小端存储即可,在大端机器上什么都不做,
 * 
 * @tparam T 
 * @param t 
 * @return T 
 */
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

/**
 * @brief 无需处理
 * 
 * @tparam T 
 * @param t 
 * @return T 
 */
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}

#endif