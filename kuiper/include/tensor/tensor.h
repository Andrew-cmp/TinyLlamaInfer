#ifndef KUIPER_INCLUDE_TENSOR_TENSOR_H_
#define KUIPER_INCLUDE_TENSOR_TENSOR_H_
#include <driver_types.h>
#include <glog/logging.h>
#include <armadillo>
#include <memory>
#include <vector>
#include "base/base.h"
#include "base/buffer.h"
namespace tensor{
//Tensor居然还包含了buffer。
//张量是一个多维数组，用于在推理的流程中管理，传递数据，同时也能配合第二次课程的Buffer类来自动管理内存或者显存资源。

//为什么Tensor中有std::shared_ptr<Buffer>来保存数据呢，假设有以下的流程：
// tensor::Tensor t1(3);
// tensor::Tensor t2 = t1;
//在第一行代码中，Tensor t1申请了一个3 × sizeof(element)大小的内存，在第二行随后用t1对t2进行赋值，所以
//std::shared_ptr<base::Buffer> buffer_;
//所以t1和t2内部共享同一块Buffer，且这块Buffer的引用计数等于2，也就是说直到这两个张量t1和t2都销毁后才会触发Buffer的析构函数进行资源释放。
// 换句话说t1中的buffer是两个张量共有的，只有当使用这块buffer的tensor都销毁后才会被释放。
//{
//   tensor::Tensor t1(3);
//   tensor::Tensor t2 = t1;
// }
//我们再举一个例子，当代码流程执行到括号外时，因为两个张量变量因为都是类内变量，所以会在第四行对两个张量都进行销毁。这是C++ RAII的内容，局部变量退出作用域后自动释放。
class Tensor{
    explicit Tensor() = default;

    explicit Tensor(base::DataType data_type, int32_t dim0, bool need_alloc = false,
      std::shared_ptr<base::DeviceAllocator> alloc = nullptr, void* ptr = nullptr);

    explicit Tensor(base::DataType data_type, int32_t dim0, int32_t dim1, bool need_alloc = false,
          std::shared_ptr<base::DeviceAllocator> alloc = nullptr, void* ptr = nullptr);
    
    explicit Tensor(base::DataType data_type, int32_t dim0, int32_t dim1, int32_t dim2,
          bool need_alloc = false, std::shared_ptr<base::DeviceAllocator> alloc = nullptr,
          void* ptr = nullptr);
    
    explicit Tensor(base::DataType data_type, int32_t dim0, int32_t dim1, int32_t dim2, int32_t dim3,
          bool need_alloc = false, std::shared_ptr<base::DeviceAllocator> alloc = nullptr,
          void* ptr = nullptr);
    
    explicit Tensor(base::DataType data_type, std::vector<int32_t> dims, bool need_alloc = false,
      std::shared_ptr<base::DeviceAllocator> alloc = nullptr, void* ptr = nullptr);

    void to_cpu();

    void to_cuda(cudaStream_t stream = nullptr);

    bool is_empty() const;

    void init_buffer(std::shared_ptr<base::DeviceAllocator> alloc, base::DataType data_type,
                     bool need_alloc, void* ptr);

    template<typename T>
    T* ptr();

    template <typename T>
    const T* ptr() const;


    template <typename T>
    T* ptr(int64_t index);

    template <typename T>
    const T* ptr(int64_t index) const;


    void reshape(const std::vector<int32_t>& dims);

    std::shared_ptr<base::Buffer> get_buffer() const;
  
    size_t size() const;
  
    size_t byte_size() const;
  
    int32_t dims_size() const;
  
    base::DataType data_type() const;
  
    int32_t get_dim(int32_t idx) const;
  
    const std::vector<int32_t>& dims() const;
  
    std::vector<size_t> strides() const;
  
    bool assign(std::shared_ptr<base::Buffer> buffer);
  
    void reset(base::DataType data_type, const std::vector<int32_t>& dims);
  
    void set_device_type(base::DeviceType device_type);
  
    base::DeviceType device_type() const;
  
    bool allocate(std::shared_ptr<base::DeviceAllocator> allocator,
                  bool need_realloc = false);

    template <typename T>
    T& index(int64_t offset);
  
    template <typename T>
    const T& index(int64_t offset) const;
  
    tensor::Tensor clone() const;
    ///这里为什么不拥有DeviceAllocator
    //而上面的构造函数里有。
    private:
        size_t size_ = 0;
        std::vector<int> dims_;
        std::shared_ptr<base::Buffer> buffer_;
        base::DataType data_type_ = base::DataType::kDataTypeUnknown;
};
template <typename T>
T& Tensor::index(int64_t offset) {
  CHECK_GE(offset, 0);
  CHECK_LT(offset, this->size());
  T& val = *(reinterpret_cast<T*>(buffer_->ptr()) + offset);
  return val;
}

template <typename T>
const T& Tensor::index(int64_t offset) const {
  CHECK_GE(offset, 0);
  CHECK_LT(offset, this->size());
  const T& val = *(reinterpret_cast<T*>(buffer_->ptr()) + offset);
  return val;
}
//直接把最底层的ptr交出去吗，感觉有点不安全啊
template<typename T>
const T* Tensor::ptr() const {
    if(!buffer_){
        return nullptr;
    }
    //这里是用const_cast给转化来的ptr指针加上了const属性。
    return const_cast<const T*>(reinterpret_cast<T*>(buffer_->ptr()));
}
template <typename T>
T* Tensor::ptr() {
  if (!buffer_) {
    return nullptr;
  }
  return reinterpret_cast<T*>(buffer_->ptr());
}
template<typename T>
T* Tensor::ptr(int64_t index){
    CHECK(buffer_ != nullptr && buffer_->ptr() != nullptr)
    << "The data area buffer of this tensor is empty or it points to a null pointer.";
    return reinterpret_cast<T*>(buffer_->ptr())+ index;

}
template <typename T>
const T* Tensor::ptr(int64_t index) const {
  CHECK(buffer_ != nullptr && buffer_->ptr() != nullptr)
      << "The data area buffer of this tensor is empty or it points to a null pointer.";
  return const_cast<T*>(reinterpret_cast<const T*>(buffer_->ptr())) + index;
}

}
#endif