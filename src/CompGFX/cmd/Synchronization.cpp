#include "synchronization.hpp"
#include "cmd.hpp"
#include <Utility/CppUtility.hpp>

using namespace egx;

ref<Semaphore>& ISynchronization::GetOrCreateCompletionSemaphore()
{
	if (_SignalSemaphore.IsValidRef()) return _SignalSemaphore;
	_SignalSemaphore = Semaphore::FactoryCreate(_CoreInterface, cpp::Format("ISynchronizationSignalSemaphore({0})", _ClassName));
	return _SignalSemaphore;
}

void ISynchronization::Submit(VkCommandBuffer cmd)
{
	std::vector<VkSemaphore> waitSemaphores;
	std::vector<VkSemaphore> signalSemaphore;
	if (_SignalSemaphore.IsValidRef())
		signalSemaphore.push_back(_SignalSemaphore->GetSemaphore());

	for (auto waitObj : _WaitObjects)
	{
		if (waitObj->IsExecuting()) {
			waitObj->ResetExecution();
			waitSemaphores.push_back(waitObj->GetOrCreateCompletionSemaphore()->GetSemaphore());
		}
	}

	CommandBuffer::Submit(_CoreInterface, { cmd }, waitSemaphores, _WaitStageFlags, signalSemaphore,
		_Completion.IsValidRef() ? _Completion->GetFence() : nullptr);

}