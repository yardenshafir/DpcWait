#include <ntifs.h>
#include <ntddk.h>

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

EXTERN_C_END

typedef struct _DPC_WAIT_EVENT
{
    KWAIT_BLOCK WaitBlock;
    PKDPC Dpc;
    PVOID Object;
} DPC_WAIT_EVENT, *PDPC_WAIT_EVENT;

EXTERN_C
NTSTATUS
ExCreateDpcEvent (
    _Outptr_ PDPC_WAIT_EVENT* DpcWaitEvent,
    _Outptr_ PKEVENT* Event,
    _In_ PKDPC Dpc
);

EXTERN_C
NTSTATUS
ExQueueDpcEventWait (
    _In_ PDPC_WAIT_EVENT DpcWait,
    _In_ BOOLEAN WaitIfNotSignaled
);

EXTERN_C
NTSTATUS
ExCancelDpcEventWait (
    _In_ PDPC_WAIT_EVENT DpcWait
);

EXTERN_C
NTSTATUS
ExDeleteDpcEvent (
    _In_ PDPC_WAIT_EVENT DpcWait
);

static KDEFERRED_ROUTINE DpcRoutine;
PDPC_WAIT_EVENT g_DpcWait;
EX_PUSH_LOCK g_WaitLock;
KDPC g_Dpc;
PKEVENT g_Event;

static
void
DpcRoutine (
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process terminated\n");
}

void
CreateProcessNotifyRoutineEx (
    _In_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_ PPS_CREATE_NOTIFY_INFO CreateInfo
    )
{
    NTSTATUS status;
    DECLARE_CONST_UNICODE_STRING(cmdString, L"cmd.exe");
    UNREFERENCED_PARAMETER(ProcessId);

    //
    // If process name is cmd.exe, create a dpc
    // that will wait for the process to terminate
    //
    if ((CreateInfo == nullptr) ||
        (RtlSuffixUnicodeString(&cmdString, CreateInfo->ImageFileName, FALSE) == FALSE))
    {
        return;
    }

    //
    // Only wait on one process
    //
    ExAcquirePushLockExclusive(&g_WaitLock);
    if (g_DpcWait == nullptr)
    {
        KeInitializeDpc(&g_Dpc, DpcRoutine, &g_Dpc);
        status = ExCreateDpcEvent(&g_DpcWait, &g_Event, &g_Dpc);
        if (!NT_SUCCESS(status))
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "ExCreateDpcEvent failed with status: 0x%x\n", status);
            ExReleasePushLockExclusive(&g_WaitLock);
            return;
        }

        g_DpcWait->Object = (PVOID)Process;
        ExQueueDpcEventWait(g_DpcWait, TRUE);
    }
    ExReleasePushLockExclusive(&g_WaitLock);
}

VOID
DriverUnload (
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    PsSetCreateProcessNotifyRoutineEx(&CreateProcessNotifyRoutineEx, TRUE);

    //
    // Change the DPC_WAIT_EVENT structure to point back to the event,
    // cancel the wait and destroy the structure
    //
    if (g_DpcWait != nullptr)
    {
        g_DpcWait->Object = g_Event;
        ExCancelDpcEventWait(g_DpcWait);
        ExDeleteDpcEvent(g_DpcWait);
    }
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = DriverUnload;
    ExInitializePushLock(&g_WaitLock);
    return PsSetCreateProcessNotifyRoutineEx(&CreateProcessNotifyRoutineEx, FALSE);
}
