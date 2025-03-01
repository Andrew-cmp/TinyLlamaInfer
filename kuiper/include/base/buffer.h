#ifndef KUIPER_INCLUDE_BASE_BUFFER_H_
#define KUIPER_INCLUDE_BASE_BUFFER_H_
#include <memory>
#include "base/alloc.h"
namespace base{
    ///先是对内存分配器allocator进行抽象，然后是对buffer进行抽象。allocator抽象的是内存的分配，是最底层的东西。而buffer是在allocator
    //上层的位置,包含Allocator。
    //Allocator执行内存分配、释放以及拷贝的动作，buffer保存内存的地址和大小以及一系列方法。

//安全获取自身的 shared_ptr。std::enable_shared_from_this 的 shared_from_this() 方法可以安全地返回一个与已有 shared_ptr 共享控制块的指针，避免多个控制块的问题。
//对象必须已经被 std::shared_ptr 管理。以下代码会引发异常：

//为了解决A处说到的可能导致的内存泄露问题，我们设计了一个Buffer类来管理用分配器申请到的内存资源，
//通俗来讲，是为了管理何时进行内存分配，分配多大的内存，自动free不用的指针。
class Buffer:public NoCopyable, std::enable_shared_from_this<Buffer>{
    private:
        size_t byte_size_ = 0;
        DeviceType device_type_ = DeviceType::kDeviceUnknown;
        //2. ptr_ 这块内存的地址，主要有两种来源， 一种是外部直接赋值得到的， Buffer不需要对它进行管理，和它的关系是借用，不负责它的生命周期管理，这种情况下对应下方use_external的值置为true。
        //3. 另外一种是需要Buffer对这块内存进行管理的，所以use_external值为false，表示需要对它的生命周期进行管理，也就是没人使用该Buffer的时候会自动将ptr_指向的地址用对应类型的Allocator完成释放。
        void * ptr_;
        bool use_external_ = false;     //是否拥有这块数据的所有权
        std::shared_ptr<DeviceAllocator> allocator_;
    public:
        explicit Buffer() = default;

        explicit Buffer(size_t byte_size, std::shared_ptr<DeviceAllocator> allocator = nullptr,
                    void* ptr = nullptr, bool use_external = false);
        bool allocate();

        void copy_from(const Buffer& buffer) const;
        void copy_from(const Buffer* buffer) const;
        void* ptr();

        const void* ptr() const;
      
        size_t byte_size() const;
      
        std::shared_ptr<DeviceAllocator> allocator() const;
      
        DeviceType device_type() const;
      
        void set_device_type(DeviceType device_type);
      
        std::shared_ptr<Buffer> get_shared_from_this();
      
        bool is_external() const;

};


}
#endif