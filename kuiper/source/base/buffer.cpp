#include "base/buffer.h"
#include <glog/logging.h>
namespace base{
//3. 如果我们像1中说的那样，实例化的Buffer如果需要直接管理一片内存/显存，拥有所有权，则使用Buffer(32,allocator)，内存分配器会被赋值给该Buffer实例，
//等到构造函数内部调用buffer.allocate的时候会申请一块由该实例负责的内存区域，流程如下：
// 1. auto buffer = Buffer(32, allocator);
// 2. 随后在Buffer的构造函数中使用外部传入的allocator申请对应大小的显存/主存。
// 4. 这样一来就buffer就拥有了一块32字节大小的内存资源，等buffer被释放的时候 ，会连带着该内存资源一起释放。
Buffer::Buffer(size_t byte_size, std::shared_ptr<DeviceAllocator> allocator,
    void* ptr , bool use_external):
    byte_size_(byte_size),
    allocator_(allocator),
    ptr_(ptr),
    use_external_(use_external){
  if(!ptr_ && allocator_){
      device_type_ = allocator_->device_type_;
      use_external_ =false;
      ptr_ = allocator_->allocate(byte_size_);
  }
}
//如果我们这里将use_external置为false，表示当前Buffer拥有该内存，表示这块资源需要Buffer进行管理，
//那么在Buffer对象释放的时候会调用对应allocator的释放方法，自动释放这块内存。
Buffer::~Buffer(){
    if(!use_external_){
        if(ptr && allocator){
            allocator->release(ptr_);
            ptr_=nullptr;
        }
    }
}
void* Buffer::ptr() {
    return ptr_;
  }
  
  const void* Buffer::ptr() const {
    return ptr_;
  }
  
  size_t Buffer::byte_size() const {
    return byte_size_;
  }
bool Buffer::allocate(){
    if(allocator_ && byte_size_ != 0){
        use_external_ = false;
        ptr_ = allocator_->allocate(byte_size_);
        if (!ptr_) {
          return false;
        } else {
          return true;
        }
      } else {
        return false;
    }
}
std::shared_ptr<DeviceAllocator> Buffer::allocator() const {
    return allocator_;
  }

void Buffer::copy_from(const Buffer& buffer) const{
  CHECK(allocator_ != nullptr);
  CHECK(buffer.ptr_ != nullptr);

  size_t byte_size = byte_size_ < buffer.byte_size_ ? byte_size_ : buffer.byte_size_;
  const DeviceType& buffer_device = buffer.device_type();
  const DeviceType& current_device = this->device_type();
  CHECK(buffer_device != DeviceType::kDeviceUnknown &&
        current_device != DeviceType::kDeviceUnknown);
  
  if (buffer_device == DeviceType::kDeviceCPU &&
      current_device == DeviceType::kDeviceCPU) {
    return allocator_->memcpy(buffer.ptr(), this->ptr_, byte_size);
  } else if (buffer_device == DeviceType::kDeviceCUDA &&
             current_device == DeviceType::kDeviceCPU) {
    return allocator_->memcpy(buffer.ptr(), this->ptr_, byte_size,
                              MemcpyKind::kMemcpyCUDA2CPU);
  } else if (buffer_device == DeviceType::kDeviceCPU &&
             current_device == DeviceType::kDeviceCUDA) {
    return allocator_->memcpy(buffer.ptr(), this->ptr_, byte_size,
                              MemcpyKind::kMemcpyCPU2CUDA);
  } else {
    return allocator_->memcpy(buffer.ptr(), this->ptr_, byte_size,
                              MemcpyKind::kMemcpyCUDA2CUDA);
  }
}


      
DeviceType Buffer::device_type() const {
    return device_type_;
}

void Buffer::set_device_type(DeviceType device_type) {
    device_type_ = device_type;
}

std::shared_ptr<Buffer> Buffer::get_shared_from_this() {
  return shared_from_this();
}
bool Buffer::is_external() const {
  return this->use_external_;
}
}