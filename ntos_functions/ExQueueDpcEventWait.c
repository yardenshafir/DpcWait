BOOLEAN
ExQueueDpcEventWait (
    _In_ PDPC_WAIT_EVENT DpcEvent,
    _In_ BOOLEAN QueueIfSignaled
    )
{
  if (DpcEvent->WaitBlock.BlockState != WaitBlockInactive)
  {
      RtlFailFast(FAST_FAIL_INVALID_ARG);
  }
  return KeRegisterObjectDpc(DpcEvent->Event,
                             DpcEvent->Dpc,
                             &DpcEvent->WaitBlock,
                             QueueIfSignaled);
}