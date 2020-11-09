NTSTATUS
ExCreateDpcEvent (
    _Out_ PDPC_WAIT_EVENT *WaitStruct,
    _Out_ PKEVENT *Event,
    _In_ PKDPC Dpc
    )
{
    PDPC_WAIT_EVENT waitEvent;
    NTSTATUS status;
    PKEVENT event;

    event = 0;
    waitEvent = (PDPC_WAIT_EVENT)ExAllocatePool2(NonPagedPool,
                                                 sizeof(DPC_WAIT_EVENT),
                                                 'WcpD');
    if (!waitEvent)
    {    
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = ObCreateObject(KernelMode,
                            ExEventObjectType,
                            NULL,
                            KernelMode,
                            NULL,
                            sizeof(KEVENT),
                            0,
                            0,
                            &event);
    if (NT_SUCCESS(status))
    {
        KeInitializeEvent(event, SynchronizationEvent, FALSE);
        ObDeleteCapturedInsertInfo(event);
        ObfReferenceObjectWithTag(event, 'eDxE');
        ObfDereferenceObjectWithTag(event, 'tlfD');
        *WaitStruct = waitEvent;
        status = STATUS_SUCCESS;
        *Event = event;
        waitEvent->Dpc = Dpc;
        waitEvent->Event = event;
        waitEvent->WaitBlock = WaitBlockInactive;
    }
    else
    {
      ExFreePool(waitEvent);
    }
    return status;
}