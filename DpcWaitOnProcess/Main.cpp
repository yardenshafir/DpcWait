#include <ntifs.h>
#include <ntddk.h>

EXTERN_C{
    NTSTATUS
    DriverEntry(
        _In_ PDRIVER_OBJECT DriverObject,
        _In_ PUNICODE_STRING RegistryPath
    );

    VOID
    DriverUnload(
        _In_ PDRIVER_OBJECT DriverObject
    );
}

typedef struct _DPC_WAIT_EVENT
{
    KWAIT_BLOCK WaitBlock;
    PKDPC Dpc;
    PVOID Object;
} DPC_WAIT_EVENT, *PDPC_WAIT_EVENT;

NTSTATUS
ExCreateDpcEvent(
    _Out_ PDPC_WAIT_EVENT* DpcWaitEvent,
    _Out_ PKEVENT* Event,
    _In_ PKDPC Dpc
);

NTSTATUS
ExQueueDpcEventWait(
    _In_ PDPC_WAIT_EVENT DpcWait,
    _In_ BOOLEAN WaitIfNotSignaled
);

NTSTATUS
ExCancelDpcEventWait(
    _In_ PDPC_WAIT_EVENT DpcWait
);

NTSTATUS
ExDeleteDpcEvent(
    _In_ PDPC_WAIT_EVENT DpcWait
);

static KDEFERRED_ROUTINE DpcRoutine;
PDPC_WAIT_EVENT g_DpcWait;
KDPC g_Dpc;
PKEVENT g_Event;
decltype(ExCreateDpcEvent)* g_ExCreateDpcEventPtr = nullptr;
decltype(ExDeleteDpcEvent)* g_ExDeleteDpcEventPtr = nullptr;
decltype(ExQueueDpcEventWait)* g_ExQueueDpcEventWaitPtr = nullptr;
decltype(ExCancelDpcEventWait)* g_ExCancelDpcEventWaitPtr = nullptr;

static
void
DpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
    DbgPrintEx(77, 0, "Process terminated\n");
}

void CreateProcessNotifyRoutineEx (
    PEPROCESS Process,
    HANDLE ProcessId,
    PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(ProcessId);

    NTSTATUS status;
    DECLARE_CONST_UNICODE_STRING(cmdString, L"cmd.exe");

    //
    // If process name is cmd.exe, create a dpc
    // that will wait for the process to terminate
    //
    if ((!CreateInfo) || (!RtlSuffixUnicodeString(&cmdString, CreateInfo->ImageFileName, FALSE)))
    {
        return;
    }

    //
    // Only wait on one process
    //
    if (g_DpcWait == nullptr)
    {
        KeInitializeDpc(&g_Dpc, DpcRoutine, &g_Dpc);
        status = g_ExCreateDpcEventPtr(&g_DpcWait, &g_Event, &g_Dpc);
        if (!NT_SUCCESS(status))
        {
            DbgPrintEx(77, 0, "ExCreateDpcEvent failed with status: 0x%x\n", status);
            return;
        }
        g_DpcWait->Object = (PVOID)Process;
        g_ExQueueDpcEventWaitPtr(g_DpcWait, 1);
    }
}

VOID
DriverUnload(
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
        g_ExCancelDpcEventWaitPtr(g_DpcWait);
        g_ExDeleteDpcEventPtr(g_DpcWait);
    }
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DECLARE_CONST_UNICODE_STRING(createDpcEvent, L"ExCreateDpcEvent");
    DECLARE_CONST_UNICODE_STRING(deleteDpcEvent, L"ExDeleteDpcEvent");
    DECLARE_CONST_UNICODE_STRING(queueDpcEventWait, L"ExQueueDpcEventWait");
    DECLARE_CONST_UNICODE_STRING(cancelDpcEventWait, L"ExCancelDpcEventWait");

    DriverObject->DriverUnload = DriverUnload;

    g_ExCreateDpcEventPtr = (decltype(ExCreateDpcEvent)*)(MmGetSystemRoutineAddress((PUNICODE_STRING)&createDpcEvent));
    g_ExDeleteDpcEventPtr = (decltype(ExDeleteDpcEvent)*)(MmGetSystemRoutineAddress((PUNICODE_STRING)&deleteDpcEvent));
    g_ExQueueDpcEventWaitPtr = (decltype(ExQueueDpcEventWait)*)(MmGetSystemRoutineAddress((PUNICODE_STRING)&queueDpcEventWait));
    g_ExCancelDpcEventWaitPtr = (decltype(ExCancelDpcEventWait)*)(MmGetSystemRoutineAddress((PUNICODE_STRING)&cancelDpcEventWait));

    if ((g_ExCreateDpcEventPtr == nullptr) ||
        (g_ExDeleteDpcEventPtr == nullptr) ||
        (g_ExQueueDpcEventWaitPtr == nullptr) ||
        (g_ExCancelDpcEventWaitPtr == nullptr))
    {
        return STATUS_UNSUCCESSFUL;
    }

    return PsSetCreateProcessNotifyRoutineEx(&CreateProcessNotifyRoutineEx, FALSE);
}
