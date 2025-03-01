#ifndef KUIPER_INCLUDE_BASE_ALLOC_H_
#define KUIPER_INCLUDE_BASE_ALLOC_H_
#include <memory>
#include <map>
#include "base.h"
namespace base{
enum class MemcpyKind {
    kMemcpyCPU2CPU = 0,
    kMemcpyCPU2CUDA = 1,
    kMemcpyCUDA2CPU = 2,
    kMemcpyCUDA2CUDA = 3,
  };

/// @brief 只负责分配动作，不保存任何数据，有一个分配时候需要的数据类型。
class DeviceAllocator{
    
    explicit DeviceAllocator(DeviceType device_type):device_type_(device_type){}
    virtual DeviceType device_type()const{
        return device_type_;
    }
    //这里最后没有=0，因为CPUDevice不需要
    virtual void memcpy(const void * src, void *dst, size_t size)const;
    //= 0 表示这是一个纯虚函数（Pure Virtual Function） 。它的作用是定义一种接口规范，强制要求派生类必须实现该函数，否则派生类也会成为抽象类，无法实例化对象。
    virtual void* allocate(size_t size) const = 0;
    virtual void release(void* ptr)const=0;
    
    virtual void memset_zero(void* ptr, size_t byte_size, void* stream, bool need_sync = false);

    private:
        DeviceType device_type_ = DeviceType::kDeviceUnknown;
};

class CPUDeviceAllocator :public DeviceAllocator{
    public:
        explicit CPUDeviceAllocator();
        void* allocate(size_t size) const override;
        void release(void* ptr)const override;
};
class CPUDeviceAllocatorFactory{
    public:
        static std::shared_ptr<CPUDeviceAllocator> get_instance(){
            if(instance == nullptr){
                instance = std::make_shared<CPUDeviceAllocator>();
            }
            return instance;
        }

    private:
        static std::shared_ptr<CPUDeviceAllocator> instance;
};

struct CudaMemoryBuffer
{
    /* data */
    void *data;
    size_t byte_size;
    bool busy;

    CudaMemoryBuffer() = default;

    CudaMemoryBuffer(void* data, size_t byte_size, bool busy)
        : data(data), byte_size(byte_size), busy(busy) {}
};

class CUDADeviceAllocator : public DeviceAllocator{
    public:
    explicit CUDADeviceAllocator();
    
    void* allocate(size_t byte_size) const override;

    void release(void* ptr) const override;

    //这是在干啥
    private:
    mutable std::map<int, size_t> bo_busy_cnt_;
    mutable std::map<int, std::vector<CudaMemoryBuffer>> big_buffers_map_;
    mutable std::map<int, std::vector<CudaMemoryBuffer>> cuda_buffers_map_;
};
class CUDADeviceAllocatorFactor{
    public:
        static std::shared_ptr<CUDADeviceAllocator> get_instance(){
            if(!instance){
                instance = std::make_shared<CUDADeviceAllocator>();
            }
            return instance;
        }
    private:
        static std::shared_ptr<CUDADeviceAllocator> instance;
};

class DeviceAllocatorFactory{
    public:
        static std::shared_ptr<DeviceAllocator> get_instance(base::DeviceType device_type){
            if(device_type == base::DeviceType::kDeviceCPU){
                return CPUDeviceAllocatorFactory::get_instance();
            }else if (device_type == base::DeviceType::kDeviceCUDA) {
                return CPUDeviceAllocatorFactory::get_instance();
            } else {
                LOG(FATAL) << "This device type of allocator is not supported!";
                return nullptr;
            }
        }
};

}

#endif