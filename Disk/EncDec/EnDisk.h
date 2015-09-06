#ifndef __ENDISK_H__
#define __ENDISK_H__

#define ENDISK_DEVICE_FLAG         L"EnDiskFlag"
#define ENDISK_DEVICE_NAME         L"\\Device\\EnDiskDeviceName"
#define ENDISK_DEVICE_SYMLINKNAME  L"\\??\\EnDiskDevice"

typedef struct _ENDISK_DEVICE_EXTERSION{
	WCHAR EnDkiskDevicFlag[0x20];
	UNICODE_STRING ustrDeviceName;	
	UNICODE_STRING ustrSymLinkName;	
}ENDISK_DEVICE_EXTERSION,*PENDISK_DEVICE_EXTERSION;

NTSTATUS 
	EnDiskCreateDevice(IN PDRIVER_OBJECT pDriverObject);

NTSTATUS 
	EnDiskDispachRoutine(IN PDEVICE_OBJECT pDeviceObject,IN PIRP Irp);

VOID 
	EnDiskUSBIrpHookUnload(IN PDRIVER_OBJECT DriverObject);

VOID 
	EnDiskReinitializationRoutine(  
	__in struct _DRIVER_OBJECT  *DriverObject,
	__in_opt PVOID  Context,
	__in  ULONG  Count
	);

#endif