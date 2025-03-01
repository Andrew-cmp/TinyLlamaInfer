#ifndef KUIPER_INCLUDE_BASE_BASE_H_
#define KUIPER_INCLUDE_BASE_BASE_H_
#include <glog/logging.h>
#include <cstdint>
#include <string>
namespace base{

enum class DeviceType:uint8_t{
    kDeviceUnknown = 0,
    kDeviceCPU = 1,
    kDeviceCUDA = 2,
};
enum class DataType:uint8_t{
    kDataTypeUnknown = 0,
    kDataTypeFp32 = 1,
    kDataTypeInt8 = 2,
    kDataTypeInt32 = 3,
};
enum class ModelType:uint8_t{
    kModelTypeUnknown = 0,
    kModelTypeLLama2 = 1,
};
inline size_t DataTypeSize(DataType data_type){
    if (data_type == DataType::kDataTypeFp32) {
        return sizeof(float);
      } else if (data_type == DataType::kDataTypeInt8) {
        return sizeof(int8_t);
      } else if (data_type == DataType::kDataTypeInt32) {
        return sizeof(int32_t);
      } else {
        return 0;
      }
}
    //这是一个 C++ 的 禁止拷贝类（Non-copyable class）的经典实现，其核心目的是通过禁用拷贝构造函数和拷贝赋值运算符，阻止类的对象被复制或赋值。以下是逐行解析：
//核心设计目的
// 1. 禁用拷贝语义
// 拷贝构造函数和拷贝赋值运算符被显式删除（= delete），使得任何尝试拷贝该类对象的行为都会导致编译错误。
// 适用场景：
// 当类管理独占资源（如文件句柄、网络连接、硬件设备）时，拷贝可能导致资源重复释放或逻辑错误。
// 2. 继承机制
// 构造函数和析构函数声明为 protected，意味着：
// 该类不能直接实例化（因为构造函数不可公开访问）。
// 只能作为基类被其他类继承，且派生类可以正常构造和析构。

//很像编译器中的属性。
class NoCopyable{
    protected:
    NoCopyable() = default;
    ~NoCopyable() = default;
    NoCopyable(const NoCopyable&) = delete;
    NoCopyable& operator=(const NoCopyable&) = delete;
};
enum StatusCode:uint8_t{
//enum class StatusCode : uint8_t{
    kSuccess = 0,
    kFunctionUnImplement = 1,
    kPathNotValid = 2,
    kModelParseError = 3,
    kInternalError = 5,
    kKeyValueHasExist = 6,
    kInvalidArgument = 7,
};
class Status{
    public:
    //初始化时可以指定状态代码和错误信息。如果不提供参数，将创建一个表示成功状态的对象。
    Status(int code = StatusCode::kSuccess, std::string error_message = "");

    //拷贝构造函数
    Status(const Status& other) = default;

    //赋值运算符是对自身进行操作，这种对自己的状态进行改变的前面的就和返回值一样
    Status& operator=(const Status& other) = default;
    Status& operator=(int code);

    bool operator==(int code) const;
    bool operator!=(int code) const;
    operator int() const ;
    operator bool() const;

    int32_t get_err_code() const;

    const std::string& get_err_msg() const;

    void set_err_msg(const std::string& err_msg);
    private :
    int code_ = StatusCode::kSuccess;
    std::string message_;
};

namespace error{
    #define STATUS_CHECK(call)                                                                       \
        do {                                                                                         \
            const base::Status status& status = call;                                                \
            if(!status){                                                                             \
                const size_t buf_size = 512;                                                         \
                char buf[buf_size];                                                                  \
                snprintf(buf, buf_size - 1,                                                          \
                         "Infer error\n File:%s Line:%d\n Error code:%d\n Error msg:%s\n", __FILE__, \
                         __LINE__, int(status), status.get_err_msg().c_str());                       \
                LOG(FATAL) << buf;                                                                   \
            }                                                                                        \
        }while(0)                                                                                    
        
Status Success(const std::string& err_msg = "");

Status FunctionNotImplement(const std::string& err_msg = "");

Status PathNotValid(const std::string& err_msg = "");

Status ModelParseError(const std::string& err_msg = "");

Status InternalError(const std::string& err_msg = "");

Status KeyHasExits(const std::string& err_msg = "");

Status InvalidArgument(const std::string& err_msg = "");
}

//这段代码通过重载<<运算符，使Status对象能像基本类型一样直接输出错误信息，提升代码可读性和易用性。核心是通过get_err_msg()获取信息并写入流，同时返回流引用以支持链式操作。
//Status status;
//std::cout << status;  // 输出：File not found.
std::ostream& operator<<(std::ostream& os, const Status& x);
}
#endif  // KUIPER_INCLUDE_BASE_BASE_H_