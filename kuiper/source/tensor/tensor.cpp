#include "tensor/tensor.h"
#include <cuda_device_runtime_api.h>
#include <cuda_runtime.h>
#include <glog/logging.h>
#include <numeric>

namespace tensor{


//这里为什么有两种类型
//init是累积结果的初始值（决定最终结果的类型Tp）
template<typenameT, typename Tp>
static size_t reduce_dimension(T begin, T end, Tp init){
  if(begin>=end){
    return 0;
  }
  size_t size = std::accumulate(begin,end,init,std::multiplies<>());
  return size;
}


static size_t data_type_size(base::DataType data_type){
  switch (data_type){
    case base::DataType::kDataTypeFp32:{
      return 4;
    }
    case base::DataType::kDataTypeInt8:{
      return 1;
    }
    case base::DataType::kDataTypeInt32{
      return 4;
    }

    default:{
      LOG(FATAL) << "Unknown data type size for " << int(data_type);
      return 0;
    }
  }
}


//总感觉这样设计怪怪的
bool Tensor::allocate(std::shared_ptr<base::DeviceAllocator> allocator, bool need_realloc){
  //allocator是什么时候初始化的呢？
  ///allocator由用户初始化
  if(!allocator){
    LOG(ERROR)<<"The allocator parameter in the allocate function is null "
                  "pointer!";
    return false;
  }
  size_t byte_size = this->byte_size();
  if(!byte_size){
    LOG(ERROR) <<  "The byte_size parameter in the allocate function is equal to zero!";
    return false;
  }

  if (buffer_ && byte_size <= buffer_->byte_size()) {
    if (!need_realloc) {
      return true;
    }
  }
  buffer_ = std::make_shared<base::Buffer>(byte_size, allocator, nullptr);
  if (!buffer_->ptr()) {
    LOG(ERROR) << "The memory allocated is a null pointer!";
    return false;
  }
  return true;
}

//主要是将ptr指向的内存区域纳入tensor的buffer中，但是是外部的，就不想要管理。
void Tensor::init_buffer(std::shared_ptr<base::DeviceAllocator>alloc, base::DataType data_type,
                         bool need_alloc, void* ptr){
    if(!alloc && !need_alloc){
      ///这里的use_external 为什么为true；
      //这里也不太懂什么意思，进入buffer的构造函数之后也没有进行实际操作。
      

      //  如果传入一个空的allocator，表示该tensor不会对该显存进行管理
      std::shared_ptr buffer =  std::make_shared<base::Buffer>(data_type_size(data_type)*size_,nullptr,ptr,true);
      this->buffer_ = buffer;
    }else{
       // 反之，如果传入一个非空的allocator，表示该tensor会对该显存进行管理，
        // 在Tensor生命周期结束后就会释放这块显存
        //在这个分支中，如果传入了alloc参数不为空，则表示buffer对传入的ptr是管理关系，需要在所有对buffer有引用的所有tensor销毁后，自动释放ptr指针指向的内存/显存资源。
      allocate(alloc, true);
    }
}

//这玩意是怎么用的,初始化的时候就要传输allocator，就很奇怪。
//因为allocator由用户进行申请

//如何用已经有的内存赋值tensor
// float* attn_ptr = nullptr;
// cudaMallocManaged(reinterpret_cast<void**>(&attn_ptr),
//                   config_->head_num_ * config_->seq_len_ * sizeof(float));
// tensor::Tensor attn(base::DataType::kDataTypeFp32, config_->head_num_,
//                       config_->seq_len_, false, alloc_cu, attn_ptr);
//我们先是用cudaMallocManaged分配了一块显存attn_ptr，后续为了让张量attn可以自动管理这块显存，我们使用了构造函数的这一分支（第二个if）
Tensor::Tensor(base::DataType data_type, int32_t dim0, bool need_alloc,
  std::shared_ptr<base::DeviceAllocator> alloc, void* ptr)
    :data_type_(data_type) {
  dim_.push_back(dim0);
  size_ = dim0;
  if(need_alloc && alloc){
    allocate(alloc);
  } else {
    //不需要对内存进行分配，将传递进来的ptr内存指针纳入tensor的buffer中。
    if(ptr != nullptr){
      CHECK(need_alloc == false)
          << "The need_alloc is is true when ptr parameter is not a null pointer.";
      init_buffer(alloc, data_type_, need_alloc, ptr)
    }
  }
}
Tensor::Tensor(base::DataType data_type, int32_t dim0, int32_t dim1, bool need_alloc,
  std::shared_ptr<base::DeviceAllocator> alloc, void* ptr)
: data_type_(data_type) {
  dims_.push_back(dim0);
  dims_.push_back(dim1);
  size_ = dim0 * dim1;
  if (need_alloc && alloc) {
    allocate(alloc);
  } else {
    init_buffer(alloc, data_type_, need_alloc, ptr);
  }
}

Tensor::Tensor(base::DataType data_type, int32_t dim0, int32_t dim1, int32_t dim2, bool need_alloc,
  std::shared_ptr<base::DeviceAllocator> alloc, void* ptr)
: data_type_(data_type) {
  dims_.push_back(dim0);
  dims_.push_back(dim1);
  dims_.push_back(dim2);
  size_ = dim0 * dim1 * dim2;
  if (need_alloc && alloc) {
    allocate(alloc);
  } else {
    init_buffer(alloc, data_type_, need_alloc, ptr);
  }
}
Tensor::Tensor(base::DataType data_type, int32_t dim0, int32_t dim1, int32_t dim2, int32_t dim3,
  bool need_alloc, std::shared_ptr<base::DeviceAllocator> alloc, void* ptr)
: data_type_(data_type) {
  dims_.push_back(dim0);
  dims_.push_back(dim1);
  dims_.push_back(dim2);
  dims_.push_back(dim3);
  size_ = dim0 * dim1 * dim2 * dim3;
  if (need_alloc && alloc) {
    allocate(alloc);
  } else {
    init_buffer(alloc, data_type_, need_alloc, ptr);
  }
}
Tensor::Tensor(base::DataType data_type, std::vector<int32_t> dims, bool need_alloc,
  std::shared_ptr<base::DeviceAllocator> alloc, void* ptr)
: dims_(std::move(dims)), data_type_(data_type) {
  size_ = reduce_dimension(dims_.begin(), dims_.end(), 1);
  if (need_alloc && alloc) {
    allocate(alloc);
  } else {
    init_buffer(alloc, data_type_, need_alloc, ptr);
  }
}
///CPU本身的内存不需要释放吗，好像是不释放，写cuda程序的时候也不释放。

void Tensor::to_cuda(cudaStream_t stream){
  CHECK_NE(buffer_, nullptr);
  const base::DeviceType device_type = this->device_type();
  if(device_type == base::DeviceType::kDeviceUnknown){
    LOG(ERROR) << "The device type of the tensor is unknown.";
  }else if(device_type == base::DeviceType::kDeviceGPU){
    size_t byte_site = this->byte_size();
    auto cu_alloc = base::CUDADeviceAllocatorFactor::get_instance();
    auto cu_buffer = std::make_shared<base::Buffer>(byte_size, cu_alloc);
    cu_alloc->memcpy(buffer_.ptr(),cu_buffer.ptr(),byte_size, base::MemcpyKind::kMemcpyCPU2CUDA,stream);
    this->buffer_ = cu_buffer;
  }else {
    LOG(INFO) << "The device type of the tensor is already cuda.";
  }
}
void Tensor::to_cpu() {
  CHECK_NE(buffer_, nullptr);
  const base::DeviceType device_type = this->device_type();
  if(device_type == base::DeviceType::kDeviceUnknown){
    LOG(ERROR) << "The device type of the tensor is unknown.";
  }else if(device_type == base::DeviceType::kDeviceCPU){
    size_t byte_site = this->byte_size();
    auto cpu_alloc = base::CPUDeviceAllocatorFactory::get_instance();
    auto cpu_buffer = std::make_shared<base::Buffer>(byte_size, cpu_alloc);
    cpu_alloc->memcpy(buffer_.ptr(),cu_buffer.ptr(),byte_size, base::MemcpyKind::kMemcpyCUDA2CPU);
    this->buffer_ = cpu_buffer;
  }else {
    LOG(INFO) << "The device type of the tensor is already cpu.";
  }
}
size_t Tensor::size() const { return this->size_; }

int32_t Tensor::get_dim(int32_t idx) const {
  CHECK_GE(idx, 0);
  CHECK_LT(idx, this->dims_.size());
  return this->dims_.at(idx);
}

base::DeviceType Tensor::device_type() const {
  if (!buffer_) {
    return base::DeviceType::kDeviceUnknown;
  }
  return buffer_->device_type();
}
bool Tensor::assign(std::shared_ptr<base::Buffer> buffer) {
  if (!buffer) {
    LOG(ERROR) << "The buffer parameter in the assign function is null pointer!";
    return false;
  }
  if (buffer_) {
    if (buffer_->device_type() != buffer->device_type()) {
      LOG(ERROR) << "The device type of the new buffer is different from the original one.";
    }
  }

  size_t byte_size = this->byte_size();
  if (byte_size > buffer->byte_size()) {
    LOG(ERROR) << "The size of buffer is too small for the tensor!";
    return false;
  }
  buffer_ = buffer;
  return true;
}
void Tensor::reshape(const std::vector<int32_t>& dims) {
  size_t size = reduce_dimension(dims.begin(), dims.end(), 1);
  if (!buffer_) {
    this->dims_ = dims;
    this->size_ = size;
    return;
  }

  if (size > size_) {
    auto new_buffer = std::make_shared<base::Buffer>(size * base::DataTypeSize(this->data_type_),
                                                     buffer_->allocator());
    CHECK(new_buffer->allocate());
    new_buffer->copy_from(buffer_.get());
    this->buffer_ = new_buffer;
  }
  this->dims_ = dims;
  this->size_ = size;
}
std::shared_ptr<base::Buffer> Tensor::get_buffer() const { return buffer_; }

Tensor Tensor::clone() const {
  Tensor new_tensor = *this;
  size_t byte_size = this->byte_size();

  auto allocator = buffer_->allocator();
  new_tensor.buffer_ = std::make_shared<base::Buffer>(byte_size, allocator);
  new_tensor.buffer_->copy_from(buffer_.get());
  return new_tensor;
}
size_t Tensor::byte_size() const { return this->size() * DataTypeSize(data_type_); }

//这是干什么的
//dim = （4，5，2，6）
//stride = （5*2*6，2*6，6，1）
std::vector<size_t> Tensor::strides() const {
  std::vector<size_t> strides;
  if (!dims_.empty()) {
    for (int32_t i = 0; i < dims_.size() - 1; ++i) {
      size_t stride = reduce_dimension(dims_.begin() + i + 1, dims_.end(), 1);
      strides.push_back(stride);
    }
    strides.push_back(1);
  }
  return strides;
}



}