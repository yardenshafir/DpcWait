BOOLEAN  
KeRegisterObjectDpc ( 
    _In_ PVOID Object,
    _In_ PRKDPC Dpc,
    _In_ PKWAIT_BLOCK WaitBlock,
    _In_ BOOLEAN QueueIfSignaled
    )
{
    KIRQL oldIrql;
    BOOLEAN result;

    WaitBlock->WaitType = WaitDpc;
    WaitBlock->BlockState = WaitBlockActive;
    WaitBlock->Dpc = Dpc;
    WaitBlock->Object = SystemArgument1;

    oldIrql = KeRaiseIrqlToDispatchLevel();
    KiAcquireKobjectLock(Object);

    if ((SystemArgument1->Header.SignalState <= 0) || 
        (KiWaitSatisfyOther(SystemArgument1), result = 1, QueueIfSignaled))
  {
    InsertTailList(&SystemArgument1->Header.WaitListHead, &SystemArgument2->WaitListEntry);
    KiReleaseKobjectLock(Object);
    KeLowerIrql(oldIrql)
  }
  else
  {
    SystemArgument2->BlockState = WaitBlockInactive;
    KeInsertQueueDpc(Dpc, SystemArgument1, SystemArgument2);
    KiReleaseKobjectLock(Object);
    KiExitDispatcher(KeGetCurrentPrcb(), 0, 1, 0, oldIrql);
  }
  return result;
}