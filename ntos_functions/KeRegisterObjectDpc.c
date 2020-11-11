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
    PKEVENT event;

    event = (PKEVENT)Object;

    WaitBlock->WaitType = WaitDpc;
    WaitBlock->BlockState = WaitBlockActive;
    WaitBlock->Dpc = Dpc;
    WaitBlock->Object = Object;

    oldIrql = KeRaiseIrqlToDispatchLevel();
    KiAcquireKobjectLock(event);

    if (event->Header.SignalState > 0)
    {
        KiWaitSatisfyOther(event);

        if (QueueIfSignaled == FALSE)
        {        
            WaitBlock->BlockState = WaitBlockInactive;
            KeInsertQueueDpc(Dpc, event, WaitBlock);

            KiReleaseKobjectLock(event);
            KiExitDispatcher(KeGetCurrentPrcb(), 0, AdjustUnwait, IO_NO_INCREMENT, oldIrql);

            return TRUE;
        }
    }

    InsertTailList(&event->Header.WaitListHead, &WaitBlock->WaitListEntry);

    KiReleaseKobjectLock(event);
    KeLowerIrql(oldIrql)
    return FALSE;
}
