#include <ntddk.h>
#include "EnDisk.h"
#include "USBIrpHook.h"
#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

NTSTATUS EnDiskCreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PENDISK_DEVICE_EXTERSION pDevExt;
	UNICODE_STRING symLinkName;
	UNICODE_STRING devName;

	RtlInitUnicodeString(&devName,ENDISK_DEVICE_NAME);
	
	status = IoCreateDevice( pDriverObject,sizeof(ENDISK_DEVICE_EXTERSION),&devName,
			 FILE_DEVICE_UNKNOWN,0,TRUE,&pDevObj );
	if(!NT_SUCCESS(status))
	{
		return status;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PENDISK_DEVICE_EXTERSION)pDevObj->DeviceExtension;
	pDevExt->ustrDeviceName = devName;
	RtlZeroMemory(pDevExt->EnDkiskDevicFlag,sizeof(pDevExt->EnDkiskDevicFlag));
	wcscpy(pDevExt->EnDkiskDevicFlag,ENDISK_DEVICE_FLAG);
	RtlInitUnicodeString(&symLinkName,ENDISK_DEVICE_SYMLINKNAME);
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName,&devName );
	if(!NT_SUCCESS(status)) 
	{
		IoDeleteDevice( pDevObj );
		return status;
	}

	pDevObj->Flags |= DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}
NTSTATUS EnDiskDispachRoutine(PDEVICE_OBJECT pDeviceObject,PIRP Irp)
{
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

VOID EnDiskUSBIrpHookUnload(IN PDRIVER_OBJECT DriverObject)
{	
	PENDISK_DEVICE_EXTERSION pDevExt;
	EnDiskHookUsbIrp(FALSE);
	
	if(DriverObject->DeviceObject)
	{
		pDevExt = (PENDISK_DEVICE_EXTERSION)DriverObject->DeviceObject->DeviceExtension;
		IoDeleteSymbolicLink(&pDevExt->ustrSymLinkName);
		IoDeleteDevice(DriverObject->DeviceObject);
	}
}

VOID EnDiskReinitializationRoutine(  
	__in struct _DRIVER_OBJECT  *DriverObject,
	__in_opt PVOID  Context,
	__in ULONG  Count
	)
{
	EnDiskHookUsbIrp(TRUE);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	int i;
//#if DBG
//	__asm int 3
//#endif
	
	EnDiskInitEnControl();
	EnDiskHookUsbIrp(TRUE);

	DriverObject->DriverUnload = EnDiskUSBIrpHookUnload;
	for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
	   DriverObject->MajorFunction[i] = EnDiskDispachRoutine;
	}
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EnDiskDeviceControlRoutine;
	EnDiskCreateDevice(DriverObject);

	IoRegisterBootDriverReinitialization(DriverObject,EnDiskReinitializationRoutine,NULL);
	return STATUS_SUCCESS;
}