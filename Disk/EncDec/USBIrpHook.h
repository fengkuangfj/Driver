#ifndef __USBIRPHOOK_H__
#define __USBIRPHOOK_H__

#define USB_DRIVER_NAME            L"\\Driver\\Disk"
#define ATTCHED_DRIVER_NAME        L"\\Driver\\PartMgr"
#define FXDEV_NAME                  L"\\Device\\HardDisk"
#define ENCRYPT_FLAGE  0x980e
#define SECTOR_SIZE    0x200
#define ENCRYPT_FLAGE_OFFSET  0x1fe
#define FIRST_SECTOR_END_FLAGE 0xaa55
NTSTATUS
ObReferenceObjectByName (
    __in PUNICODE_STRING ObjectName,
    __in ULONG Attributes,
    __in_opt PACCESS_STATE AccessState,
    __in_opt ACCESS_MASK DesiredAccess,
    __in POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode,
    __inout_opt PVOID ParseContext,
    __out PVOID *Object
    );

NTSTATUS
  ObQueryNameString(
    IN PVOID  Object,
    OUT POBJECT_NAME_INFORMATION  ObjectNameInfo,
    IN ULONG  Length,
    OUT PULONG  ReturnLength
    ); 

VOID 
	EnDiskInitEnControl();

VOID  
	EnDiskOpenEnControl();

VOID  
	EnDiskCloseEnControl();

BOOLEAN  
	EnDiskGetEnControlStatus();

NTSTATUS 
	EnDiskKernelReadSector(
	IN  PDEVICE_OBJECT DeviceObject,
	IN  LARGE_INTEGER  SectorOffset,
	IN  ULONG  SectorSize,
	IN  ULONG  SectorCount,
	OUT PUCHAR  SectorData,
	OUT ULONG_PTR  *SectorDataLen);

BOOLEAN 
	EnDiskIsEnDisk(IN ULONG DeviceNumber);

NTSTATUS
	EnDiskDeviceControlRoutine(IN PDEVICE_OBJECT pDeviceObject,IN PIRP Irp);

BOOLEAN 
	EnDiskIsDiskDeviceObject(IN PDEVICE_OBJECT CurrentDeviceObject);

PDRIVER_OBJECT
	EnDiskGetDriverObject(IN UNICODE_STRING Unicode_String);

NTSTATUS 
	EnDiskNewIoCompletion(IN PDEVICE_OBJECT  DeviceObject,PIRP  Irp,IN PVOID  Context);

NTSTATUS  
	EnDiskNewReadDispatch(IN PDEVICE_OBJECT  pDeviceObject  , IN PIRP  irp);

VOID 
	EnDiskHookUsbIrp(IN BOOLEAN hook);
#endif