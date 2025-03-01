#include <glog/logging.h>
#include <cstdlib>
#include "base/alloc.h"
#if (defined(_POSIX_ADVISORY_INFO) && (_POSIX_ADVISORY_INFO >= 200112L))
#define KUIPER_HAVE_POSIX_MEMALIGN
#endif
namespace base{
CPUDeviceAllocator::CPUDeviceAllocator() : DeviceAllocator(DeviceType::kDeviceCPU) {
}
CPUDeviceAllocator::release(void* ptr)const{
    if(ptr!=nullptr){
        free(ptr);
    }
}
CPUDeviceAllocator::allocate(size_t size)const{
    if (!byte_size) {
        return nullptr;
      }
    #ifdef KUIPER_HAVE_POSIX_MEMALIGN
      void * data;
      const size_t alignment = (byte_size>=size_t(1024))? size_t(32):size_t(16);
      int status = posix_memalign((void**)&data,
                                  (alignment >= sizeof(void*))?alignment :sizeof(void*),
                                   size
                                    );
    if(status != 0)return nullptr;
    return data;
    #else
      void *data = malloc(size);
      return data;
    #endif
}

std::shared_ptr<CPUDeviceAllocator> CPUDeviceAllocator::instance = nullptr;
}