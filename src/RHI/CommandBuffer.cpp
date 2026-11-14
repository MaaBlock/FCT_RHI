#include "CommandBuffer.h"
#include "RayTracingPipeline.h"

#include "Fence.h"
#include "Semaphore.h"

namespace FCT{
     namespace RHI {
          CommandBuffer::~CommandBuffer()
          {
               for (auto& desc : m_waitSemaphores)
               {
                    desc.semaphore->release();
               }
               for (auto& semaphore : m_signalSemaphores)
               {
                    semaphore->release();
               }
               if (m_fence)
               {
                    m_fence->release();
               }
          }

          void CommandBuffer::fence(Fence* fence)
          {
               FCT_SAFE_RELEASE(m_fence);
               m_fence = fence;
               FCT_SAFE_ADDREF(m_fence);
          }

          void CommandBuffer::clearWaitSemaphores()
          {
               for (auto& desc : m_waitSemaphores)
               {
                    desc.semaphore->release();
               }
               m_waitSemaphores.clear();
          }

          void CommandBuffer::addWaitSemaphore(Semaphore* semaphore, PipelineStages stage)
          {
               WaitSemaphoreDescription desc;
               desc.semaphore = semaphore;
               desc.stages = stage;
               semaphore->addRef();
               m_waitSemaphores.push_back(desc);
          }

          void CommandBuffer::addSignalSemaphore(Semaphore* semaphore)
          {
               semaphore->addRef();
               m_signalSemaphores.push_back(semaphore);
          }

          void CommandBuffer::traceRays(
              const StridedDeviceAddressRegions& regions,
              uint32_t width,
              uint32_t height,
              uint32_t depth)
          {
              traceRays(&regions.rayGen, &regions.miss, &regions.hitGroup, &regions.callable, width, height, depth);
          }
     }
}
