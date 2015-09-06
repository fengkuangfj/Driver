#include <ntddk.h>
#include"jgg.h"
#include "EnDiskComm.h"
#include "USBIrpHook.h"
#include "EnDisk.h"
PDRIVER_DISPATCH OldReadWirteDispatch = NULL;
extern POBJECT_TYPE *IoDriverObjectType;
static LONG EnControl = 0;

PDRIVER_DISPATCH OldReadDispatch = NULL;


VOID  EnDiskInitEnControl()
{
	InterlockedExchangePointer((volatile PVOID *)&EnControl,0);
}
VOID  EnDiskOpenEnControl()
{
	InterlockedIncrement(&EnControl);
}
VOID  EnDiskCloseEnControl()
{
    InterlockedDecrement (&EnControl);
}

BOOLEAN  EnDiskGetEnControlStatus()
{
	return EnControl > 0;
}

NTSTATUS EnDiskKernelReadSector(
	IN  PDEVICE_OBJECT DeviceObject,
	IN  LARGE_INTEGER  SectorOffset,
	IN  ULONG  SectorSize,
	IN  ULONG  SectorCount,
	OUT PUCHAR  SectorData,
	OUT ULONG_PTR  *SectorDataLen)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG  readSize = 0;
	PUCHAR readBuffer = (PUCHAR) NULL;
	KEVENT event;
	PIRP  irp = NULL;
	IO_STATUS_BLOCK ioStatus;

    if(SectorSize >= 512) {
        readSize = SectorSize * SectorCount;
    } else {
        readSize = 512 * SectorCount;
    }

    readBuffer = (PUCHAR)ExAllocatePoolWithTag( NonPagedPoolCacheAligned,readSize,'btsF' );
    if (readBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

	KeInitializeEvent( &event, NotificationEvent, FALSE );
	RtlZeroMemory(readBuffer, readSize);

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ, DeviceObject,readBuffer,
          readSize, &SectorOffset,&event,&ioStatus );
    if(!irp){
		ExFreePool(readBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }else{
        PIO_STACK_LOCATION irpStack;
        irpStack = IoGetNextIrpStackLocation(irp);
        irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
	}

    status = IoCallDriver( DeviceObject, irp );
    if(status == STATUS_PENDING) 
	{
        KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,(PLARGE_INTEGER) NULL);
        status = ioStatus.Status;
    }
	if(*SectorDataLen >= ioStatus.Information){
		RtlCopyMemory(SectorData,readBuffer,ioStatus.Information);
		*SectorDataLen =  ioStatus.Information;
	}else{   
		RtlZeroMemory(readBuffer,*SectorDataLen);
		*SectorDataLen = 0;
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	if(readBuffer)
	{
	  ExFreePool(readBuffer);
	}
	return  status;
}

BOOLEAN EnDiskIsEnDisk(ULONG DeviceNumber)
{
	UNICODE_STRING  ustrFxDevName;
	PDEVICE_OBJECT  pDeviceObject;
	PDRIVER_OBJECT  pDriverObject;
	UNICODE_STRING  DiskDriverName;
	ULONG           ReturnLength;
	BOOLEAN         ret = FALSE;
	WCHAR FxDevName[0x40] = FXDEV_NAME;
	POBJECT_NAME_INFORMATION  pDeviceName;
	PUCHAR         SectorData  = NULL;
	ULONG_PTR          RetLen  = 0;
	LARGE_INTEGER SectorOffset = {0};

	FxDevName[wcslen(FxDevName)] = (WCHAR)(DeviceNumber + '0');
	FxDevName[wcslen(FxDevName)] = '\0';
	RtlInitUnicodeString(&ustrFxDevName,FxDevName);
	RtlInitUnicodeString(&DiskDriverName,USB_DRIVER_NAME);
	pDriverObject = EnDiskGetDriverObject(DiskDriverName);

	if(!pDriverObject)
	{
		return ret;
	}
	SectorData  = (PUCHAR)ExAllocatePoolWithTag( NonPagedPoolCacheAligned,SECTOR_SIZE,'btsF' );
	if(!SectorData)
	{
       return ret;
	}

	try{
		EnDiskOpenEnControl();
		for(pDeviceObject = pDriverObject->DeviceObject; pDeviceObject != NULL; pDeviceObject = pDeviceObject->NextDevice)
		{
			if(!EnDiskIsDiskDeviceObject(pDeviceObject))
			{
				continue;
			}
			if(ObQueryNameString( pDeviceObject,NULL,0,&ReturnLength)==  STATUS_INFO_LENGTH_MISMATCH)
			{
			   pDeviceName = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool,ReturnLength,'DPN1');
			   if(pDeviceName && NT_SUCCESS(ObQueryNameString( pDeviceObject,pDeviceName,ReturnLength,&ReturnLength)))
				{
					pDeviceName->Name.Length = (USHORT)wcslen(FxDevName)*sizeof(WCHAR);
					if(!RtlCompareUnicodeString(&pDeviceName->Name,&ustrFxDevName,TRUE))
					{
						RtlZeroMemory(SectorData,SECTOR_SIZE);
						RetLen = SECTOR_SIZE;
						if(NT_SUCCESS(EnDiskKernelReadSector(pDeviceObject,SectorOffset,SECTOR_SIZE,1,SectorData,&RetLen)))
						{
							if(*((USHORT*)(SectorData+ENCRYPT_FLAGE_OFFSET)) == ENCRYPT_FLAGE){
								ret = TRUE;
							}
						}	
					}
				}
			   if(pDeviceName)
				{
					ExFreePool(pDeviceName);
					pDeviceName = NULL;
				}
			}
			if(ret)
			{
				break;
			}
		}

	}finally{
		EnDiskCloseEnControl();
	}
	if(SectorData)
	{
       ExFreePool(SectorData);
	}
	return ret;
}


NTSTATUS EnDiskDeviceControlRoutine(PDEVICE_OBJECT pDeviceObject,PIRP Irp)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG    RetDataLen = 0;
	ULONG    InDatalen = 0;
	ULONG    OutDatlen = 0;
	PIO_STACK_LOCATION  Irps = NULL;
	PENDISK_QUERYVOLUME  InData = NULL;
	PENDISK_DEVICE_EXTERSION DeviceExtersion = NULL;

	DeviceExtersion = (PENDISK_DEVICE_EXTERSION)pDeviceObject->DeviceExtension;
  
	if(wcscmp(DeviceExtersion->EnDkiskDevicFlag,ENDISK_DEVICE_FLAG))
	{
		Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = RetDataLen;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
		return Irp->IoStatus.Status;
	}
	Irps = IoGetCurrentIrpStackLocation(Irp);
	switch(Irps->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_ENDISK_QUERYVOLUME:
			{
				InDatalen = Irps->Parameters.DeviceIoControl.InputBufferLength;
				OutDatlen = Irps->Parameters.DeviceIoControl.OutputBufferLength;
				InData = (PENDISK_QUERYVOLUME)Irp->AssociatedIrp.SystemBuffer;
				if(!InData || InData->TypeSize != sizeof(ENDISK_QUERYVOLUME) || InDatalen != sizeof(ENDISK_QUERYVOLUME) || OutDatlen !=sizeof(ENDISK_QUERYVOLUME))
				{
					status = STATUS_UNSUCCESSFUL;
					break;
				}
				if(EnDiskIsEnDisk(InData->DeviceNumber)){
					InData->IsEnDisk = TRUE;
					RetDataLen = OutDatlen;
				}else{
					InData->IsEnDisk = FALSE;
					RetDataLen = 0;
				}
				break;
			}
		default:
			status = STATUS_INVALID_VARIANT; 
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = RetDataLen;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}
BOOLEAN EnDiskIsDiskDeviceObject(PDEVICE_OBJECT CurrentDeviceObject)
{
	BOOLEAN ret = FALSE;
	PUNICODE_STRING AttchedDriverName = NULL;
	PDRIVER_OBJECT  AttchedDriverObject = NULL;
	UNICODE_STRING  unAttchedDriverName;

	if(!CurrentDeviceObject)
	{
		return FALSE;
	}
	if(CurrentDeviceObject->AttachedDevice)
	{
		AttchedDriverObject = CurrentDeviceObject->AttachedDevice->DriverObject;
		AttchedDriverName = AttchedDriverObject->DriverName.Length ? &AttchedDriverObject->DriverName : NULL;
	}
	if(AttchedDriverName)
	{
		RtlInitUnicodeString(&unAttchedDriverName,ATTCHED_DRIVER_NAME);
		ret = !RtlCompareUnicodeString(AttchedDriverName,&unAttchedDriverName,TRUE);
	}
	return ret;

}

PDRIVER_OBJECT EnDiskGetDriverObject(UNICODE_STRING Unicode_String)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDRIVER_OBJECT Driver_Object;

    status = ObReferenceObjectByName(&Unicode_String,OBJ_CASE_INSENSITIVE,NULL, 
             0,*IoDriverObjectType,KernelMode,NULL, (PVOID*)&Driver_Object); 
    if(!NT_SUCCESS(status)){
      DbgPrint("无法获得驱动对象\n");
      return NULL;
    }else{
      ObDereferenceObject(Driver_Object);
	  return Driver_Object;
    }
}

NTSTATUS EnDiskNewIoCompletion(PDEVICE_OBJECT  DeviceObject,PIRP  Irp,PVOID  Context)
{
	PIO_STACK_LOCATION irpStack;
	PUCHAR buffer = NULL;
	PUCHAR SwpBuffer = NULL;
	NTSTATUS  status = Irp->IoStatus.Status;
	UCHAR Key[4] = {0};
	Key[0] = 0x12;
	Key[1] = 0x34;
	Key[2] = 0x56;
	Key[3] = 0x78;

	
	//KdPrint(("EnDiskNewIoCompletion...\n"));
	if(Irp->MdlAddress && !EnDiskGetEnControlStatus())
	{
		buffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);
		if(*((USHORT*)(buffer+ENCRYPT_FLAGE_OFFSET)) == ENCRYPT_FLAGE )
		{  
			SwpBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool,0x200,'new1');
			if(SwpBuffer)
			{
				RtlCopyMemory(SwpBuffer,buffer,SECTOR_SIZE);
				Jgg_Decrypt(Key,4,SwpBuffer,buffer,SECTOR_SIZE);
				*((USHORT*)(buffer+ENCRYPT_FLAGE_OFFSET)) = 0xAA55;
				ExFreePool(SwpBuffer);
			}
		}
	}
	if (Irp->PendingReturned && Irp->CurrentLocation <= Irp->StackCount){
        IoMarkIrpPending(Irp);
    }
    return STATUS_SUCCESS;

}

NTSTATUS  EnDiskNewReadDispatch(PDEVICE_OBJECT  pDeviceObject  , PIRP  irp)
{
	PIO_STACK_LOCATION irpStack;
	ULONG length = 0;
	NTSTATUS status;
	PUNICODE_STRING AttchedDriverName = NULL;
    irpStack = IoGetCurrentIrpStackLocation(irp);
	if(irpStack->Parameters.Read.ByteOffset.QuadPart == 0)
	{
		length = irpStack->Parameters.Read.Length;
		if(EnDiskIsDiskDeviceObject(pDeviceObject))
		{
			irpStack->Control = SL_INVOKE_ON_SUCCESS|SL_INVOKE_ON_ERROR|SL_INVOKE_ON_CANCEL;
			irpStack->CompletionRoutine = (PIO_COMPLETION_ROUTINE)EnDiskNewIoCompletion;
		}
	}
	status = OldReadDispatch(pDeviceObject,irp);
	return status;
}
NTSTATUS  EnDiskNewWriteDispatch(PDEVICE_OBJECT  pDeviceObject  , PIRP  irp)
{
	PIO_STACK_LOCATION irpStack;
	ULONG length = 0;
	NTSTATUS status;
	PUNICODE_STRING AttchedDriverName = NULL;
	PUCHAR buffer = NULL;
	
	LARGE_INTEGER SectorOffset = {0};
	PUCHAR        SectorData  = NULL;
	ULONG_PTR         RetLen;

	UCHAR Key[4] = {0};
	Key[0] = 0x12;
	Key[1] = 0x34;
	Key[2] = 0x56;
	Key[3] = 0x78;

    irpStack = IoGetCurrentIrpStackLocation(irp);
	if(irpStack->Parameters.Write.ByteOffset.QuadPart == 0)
	{
//#ifdef DBG
//		__asm int 3
//#endif
		length = irpStack->Parameters.Write.Length;
		if(EnDiskIsDiskDeviceObject(pDeviceObject)&& irp->MdlAddress )
		{
			buffer = (PUCHAR)MmGetSystemAddressForMdlSafe(irp->MdlAddress,NormalPagePriority);
			if(buffer &&(*((USHORT*)(buffer+ENCRYPT_FLAGE_OFFSET)) == FIRST_SECTOR_END_FLAGE) )
			{  
				SectorData  = (PUCHAR)ExAllocatePoolWithTag( NonPagedPoolCacheAligned,SECTOR_SIZE,'btsF' );
				RetLen = SECTOR_SIZE;
				try {
					EnDiskOpenEnControl();
					if(SectorData && NT_SUCCESS(EnDiskKernelReadSector(pDeviceObject,SectorOffset,SECTOR_SIZE,1,SectorData,&RetLen)))
					{
						if(*((USHORT*)(SectorData+ENCRYPT_FLAGE_OFFSET)) == ENCRYPT_FLAGE){
							RtlCopyMemory(buffer,SectorData,SECTOR_SIZE);
						}
					}
				} finally {
					EnDiskCloseEnControl();
				}
				if(SectorData) 
				{
					ExFreePool(SectorData);
					SectorData = NULL;
				}
			}
		}
	}
	status = OldReadWirteDispatch(pDeviceObject,irp);
	return status;
}
VOID EnDiskHookUsbIrp(BOOLEAN hook)
{
	UNICODE_STRING  DiskDriverName;
	PDRIVER_OBJECT  DiskDeviceObject = NULL;
	BOOLEAN         ret;
	RtlInitUnicodeString(&DiskDriverName,USB_DRIVER_NAME);
	DiskDeviceObject = EnDiskGetDriverObject(DiskDriverName);
	if(DiskDeviceObject)
	{
		if(hook){
			if(DiskDeviceObject->MajorFunction[IRP_MJ_READ] != EnDiskNewReadDispatch)
			{
			   OldReadDispatch = DiskDeviceObject->MajorFunction[IRP_MJ_READ];
               InterlockedExchangePointer((PVOID *)&DiskDeviceObject->MajorFunction[IRP_MJ_READ],(PVOID)EnDiskNewReadDispatch);
			}
			if(DiskDeviceObject->MajorFunction[IRP_MJ_WRITE]  != EnDiskNewWriteDispatch)
			{
				OldReadWirteDispatch = DiskDeviceObject->MajorFunction[IRP_MJ_WRITE];
               InterlockedExchangePointer((PVOID *)&DiskDeviceObject->MajorFunction[IRP_MJ_WRITE],(PVOID)EnDiskNewWriteDispatch);
			}
		}else{
			if(OldReadDispatch)
			{
				 InterlockedExchangePointer((PVOID *)&DiskDeviceObject->MajorFunction[IRP_MJ_READ],OldReadDispatch);
			}
			if(OldReadWirteDispatch)
			{
				 InterlockedExchangePointer((PVOID *)&DiskDeviceObject->MajorFunction[IRP_MJ_WRITE],OldReadWirteDispatch);
			}
		}
	}
}







