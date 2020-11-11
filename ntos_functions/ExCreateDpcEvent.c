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

    event = NULL;
    waitEvent = (PDPC_WAIT_EVENT)ExAllocatePool(NonPagedPool,
                                                sizeof(DPC_WAIT_EVENT),
                                                'WcpD');
    if (waitEvent == NULL)
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
        ObReferenceObjectWithTag(event, 'eDxE');
        ObDereferenceObject(event);

        waitEvent->Dpc = Dpc;
        waitEvent->Event = event;
        waitEvent->WaitBlock = WaitBlockInactive;

        status = STATUS_SUCCESS;
        *WaitStruct = waitEvent;
        *Event = event;
    }
    else
    {
        ExFreePool(waitEvent);
    }
    return status;
}
