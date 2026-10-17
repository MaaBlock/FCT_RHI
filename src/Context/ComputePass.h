#ifndef FCT_COMPUTE_PASS_H
#define FCT_COMPUTE_PASS_H

#include "../Base/Status.h"
#include "../MutilThreadBase/RefCount.h"
#include <string>
#include <memory>

namespace FCT {

    class Image;

    namespace RHI {
        class CommandBuffer;
    }

    class ComputeExecutionContext {
    public:
        virtual ~ComputeExecutionContext() = default;

        virtual Image* getImage(const std::string& name) = 0;
        virtual void* getExternalNativeHandle(const std::string& name) = 0;
        virtual RHI::CommandBuffer* getCommandBuffer() = 0;
    };

    class IComputeDispatcher : public RefCount {
    public:
        ~IComputeDispatcher() override = default;

        virtual Status dispatch(ComputeExecutionContext& context) = 0;
    };

} // namespace FCT

#endif // FCT_COMPUTE_PASS_H
