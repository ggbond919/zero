#ifndef __ZERO_BYTEARRAY_H__
#define __ZERO_BYTEARRAY_H__

#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

namespace zero {

/**
 * @brief 二进制数组，提供基础类型的序列化和反序列化
 * 
 */
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief bytearray内存存储节点
     * 
     */
    struct Node {
        Node(size_t s);
        Node();
        ~Node();
        char* ptr;
        Node* next;
        size_t size;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

public:
    /**
     * @brief 写入固定长度数据
     * 
     * @param value 
     */
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);

    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);

    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    /**
     * @brief 写入Varint类型的数据
     * 
     * @param value 
     */
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);

    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    /**
     * @brief 写入浮点类型数据
     * 
     * @param value 
     */
    void writeFloat(float value);
    void writeDouble(double value);


    /**
     * @brief 写入string类型数据，长度为无符号整型
     * 
     * @param value 
     */
    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);

    /**
     * @brief 写入string类型数据，用无符号Varint64作为长度类型
     * 
     * @param value 
     */
    void writeStringVint(const std::string& value);

    /**
     * @brief 写入string类型数据，无长度限制
     * 
     * @param value 
     */
    void writeStringWithoutLength(const std::string& value);
public:
    /**
     * @brief 固定长度整型读取
     * 
     * @return int8_t 
     */
    int8_t readFint8();
    uint8_t readFuint8();

    int16_t readFint16();
    uint16_t readFuint16();

    int32_t readFint32();
    uint32_t readFuint32();

    int64_t readFint64();
    uint64_t readFuint64();

    /**
     * @brief 变长整型读取
     * 
     * @return int32_t 
     */
    int32_t readInt32();
    uint32_t readUint32();

    int64_t readInt64();
    uint64_t readUint64();

    /**
     * @brief 浮点型读取
     * 
     * @return float 
     */
    float readFloat();
    double readDouble();

    /**
     * @brief string类型数据读取，用无符号整型作为长度
     * 
     * @return std::string 
     */
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();

    /**
     * @brief 
     * 
     * @return 读取std::string类型的数据,用无符号Varint64作为长度 
     */
    std::string readStringVint();

public:
    void clear();
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;
    size_t getPosition() const { return m_position;}
    void setPosition(size_t v);
    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);
    size_t getBaseSize() const { return m_baseSize;}

    /**
    
     * @brief 获取剩余可读取的数据大小
     * 
     * @return size_t 
     */
    size_t getReadSize() const { return m_size - m_position;}
    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    /**
     * @brief 将ByteArray里面的数据[m_position, m_size)转成std::string
     * 
     * @return std::string 
     */
    std::string toString() const;

    /**
     * @brief 将ByteArray里面的数据[m_position, m_size)转成std::string 16进制
     * 
     * @return std::string 
     */
    std::string toHexString() const;
    /**
     * @brief 返回所有使用的Node节点，即可读取数据的节点
     * 
     * @param buffers 
     * @param len 
     * @return uint64_t 
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    
    /**
     * @brief 返回未被使用的Node节点
     * 
     * @param buffers 
     * @param len 
     * @return uint64_t 
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
    size_t getSize() const { return m_size;}

private:
    /**
     * @brief 扩容，增加Node节点
     * 
     * @param size 
     */
    void addCapacity(size_t size);
    
    /**
     * @brief 获取剩余容量
     * 
     * @return size_t 
     */
    size_t getCapacity() const { return m_capacity - m_position;}

private:
    /// 内存块大小
    size_t m_baseSize;
    /// 当前操作位置
    size_t m_position;
    /// 所有Node节点的总容量
    size_t m_capacity;
    /// 当前数据大小
    size_t m_size;
    /// 字节序，
    int8_t m_endian;
    /// 第一个内存块指针
    Node* m_root;
    /// 当前操作的内存块指针
    Node* m_cur;

};



}


#endif