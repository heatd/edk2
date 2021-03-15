/**
 * @file Driver entry point
 * 
 * @copyright Copyright (c) 2021 Pedro Falcato
 * 
 */

#include "Ext2.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mExt2DriverNameTable[] = {
  {
    "eng;en",
    L"Ext2 File System Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mExt2ControllerNameTable[] = {
  {
    "eng;en",
    L"Ext2 File System"
  },
  {
    NULL,
    NULL
  }
};

extern EFI_COMPONENT_NAME_PROTOCOL gExt2ComponentName;

EFI_STATUS
EFIAPI
Ext2ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
{
  // TODO: Do we need to test whether we're managing the handle, like FAT does?
  return LookupUnicodeString2(Language,
                              This->SupportedLanguages,
                              mExt2ControllerNameTable,
                              ControllerName,
                              (BOOLEAN)(This == &gExt2ComponentName));
}

EFI_STATUS
EFIAPI
Ext2ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2(Language,
                              This->SupportedLanguages,
                              mExt2DriverNameTable,
                              DriverName,
                              (BOOLEAN)(This == &gExt2ComponentName));
  // TODO: What does this last parameter? Are we testing whether we're NAME or NAME2? Investigate.
}

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gExt2ComponentName = {
  Ext2ComponentNameGetDriverName,
  Ext2ComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL gExt2ComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) Ext2ComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) Ext2ComponentNameGetControllerName,
  "en"
};

EFI_STATUS EFIAPI Ext2IsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL);

EFI_STATUS EFIAPI Ext2Bind (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL);

EFI_STATUS EFIAPI Ext2Stop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE *ChildHandleBuffer OPTIONAL
)
{
  // TODO: Implement
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Ext2Unload (IN EFI_HANDLE ImageHandle)
{
  // TODO: Implement
  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gExt2BindingProtocol = 
{
	Ext2IsBindingSupported,
	Ext2Bind,
	Ext2Stop,
	EXT2_DRIVER_VERSION
};

EFI_STATUS
EFIAPI
Ext2EntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
	EFI_STATUS st = EfiLibInstallAllDriverProtocols2(ImageHandle,
                                                   SystemTable,
                                                   &gExt2BindingProtocol,
                                                   ImageHandle,
                                                   &gExt2ComponentName,
                                                   &gExt2ComponentName2,
                                                   NULL, NULL, NULL, NULL);
  
  if(EFI_ERROR(st))
    ASSERT_EFI_ERROR(st);
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI Ext2IsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL)
{

  // Note to self: EFI_OPEN_PROTOCOL_TEST_PROTOCOL lets us not close the
  // protocol and ignore the output argument entirely

  EFI_STATUS st = gBS->OpenProtocol(ControllerHandle,
                                    &gEfiDiskIoProtocolGuid,
                                    NULL,
                                    BindingProtocol->ImageHandle,
                                    ControllerHandle,
                                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
  if(EFI_ERROR(st))
    return st;

  st = gBS->OpenProtocol(ControllerHandle,
                         &gEfiBlockIoProtocolGuid,
                         NULL,
                         BindingProtocol->ImageHandle,
                         ControllerHandle,
                         EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
 return st;
}

EFI_STATUS EFIAPI Ext2Bind (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL)
{
  EFI_DISK_IO_PROTOCOL *diskIo;
  EFI_DISK_IO2_PROTOCOL *diskIo2 = NULL;
  EFI_BLOCK_IO_PROTOCOL *blockIo;


  EFI_STATUS st = gBS->OpenProtocol(ControllerHandle,
                                    &gEfiDiskIoProtocolGuid,
                                    (VOID **) &diskIo,
                                    BindingProtocol->ImageHandle,
                                    ControllerHandle,
                                    EFI_OPEN_PROTOCOL_BY_DRIVER);
  if(EFI_ERROR(st))
    return st;

  st = gBS->OpenProtocol(ControllerHandle,
                         &gEfiDiskIo2ProtocolGuid,
                         (VOID **) &diskIo2,
                         BindingProtocol->ImageHandle,
                         ControllerHandle,
                         EFI_OPEN_PROTOCOL_BY_DRIVER);
  // It's okay to not support DISK_IO2

  st = gBS->OpenProtocol(ControllerHandle,
                         &gEfiBlockIoProtocolGuid,
                         (VOID **) &blockIo,
                         BindingProtocol->ImageHandle,
                         ControllerHandle,
                         EFI_OPEN_PROTOCOL_BY_DRIVER);

  if(EFI_ERROR(st))
    goto error;

  st = Ext2OpenPartition(diskIo, diskIo2, blockIo);

  if(!EFI_ERROR(st))
    return st;

error:
  if(diskIo)
  {
    gBS->CloseProtocol(ControllerHandle,
                       &gEfiDiskIoProtocolGuid,
                       BindingProtocol->ImageHandle,
                       ControllerHandle);
  }

  if(diskIo2)
  {
    gBS->CloseProtocol(ControllerHandle,
                       &gEfiDiskIo2ProtocolGuid,
                       BindingProtocol->ImageHandle,
                       ControllerHandle);
  }

  if(blockIo)
  {
    gBS->CloseProtocol(ControllerHandle,
                       &gEfiBlockIoProtocolGuid,
                       BindingProtocol->ImageHandle,
                       ControllerHandle);
  }

  return st;
}
