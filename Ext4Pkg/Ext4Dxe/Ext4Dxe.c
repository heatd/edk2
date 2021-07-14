/**
  @file Driver entry point

  Copyright (c) 2021 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4Dxe.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mExt4DriverNameTable[] = {
  {
    "eng;en",
    L"Ext4 File System Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mExt4ControllerNameTable[] = {
  {
    "eng;en",
    L"Ext4 File System"
  },
  {
    NULL,
    NULL
  }
};

// Needed by gExt4ComponentName*

EFI_STATUS
EFIAPI
Ext4ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
Ext4ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

extern EFI_COMPONENT_NAME_PROTOCOL  gExt4ComponentName;

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gExt4ComponentName = {
  Ext4ComponentNameGetDriverName,
  Ext4ComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gExt4ComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)Ext4ComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)Ext4ComponentNameGetControllerName,
  "en"
};

// Needed by gExt4BindingProtocol

EFI_STATUS EFIAPI
Ext4IsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL
  );

EFI_STATUS EFIAPI
Ext4Bind (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL
  );

EFI_STATUS EFIAPI
Ext4Stop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE *ChildHandleBuffer OPTIONAL
  );

EFI_DRIVER_BINDING_PROTOCOL  gExt4BindingProtocol =
{
  Ext4IsBindingSupported,
  Ext4Bind,
  Ext4Stop,
  EXT4_DRIVER_VERSION
};

EFI_STATUS
EFIAPI
Ext4ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
{
  // TODO: Do we need to test whether we're managing the handle, like FAT does?
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mExt4ControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gExt4ComponentName)
           );
}

EFI_STATUS
EFIAPI
Ext4ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mExt4DriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gExt4ComponentName)
           );
}

EFI_STATUS EFIAPI
Ext4IsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL
  );

EFI_STATUS EFIAPI
Ext4Bind (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL
  );

EFI_STATUS EFIAPI
Ext4Stop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Sfs;
  EXT4_PARTITION                   *Partition;
  BOOLEAN                          HasDiskIo2;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&Sfs,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Partition = (EXT4_PARTITION *)Sfs;

  HasDiskIo2 = Ext4DiskIo2 (Partition) != NULL;

  Status = Ext4UnmountAndFreePartition (Partition);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Partition->Interface,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Close all open protocols (DiskIo, DiskIo2, BlockIo)

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if(HasDiskIo2) {
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiDiskIo2ProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
Ext4EntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gExt4BindingProtocol,
             ImageHandle,
             &gExt4ComponentName,
             &gExt4ComponentName2,
             NULL,
             NULL,
             NULL,
             NULL
             );

  if(EFI_ERROR (Status)) {
    return Status;
  }

  return Ext4InitialiseUnicodeCollation (ImageHandle);
}

EFI_STATUS
EFIAPI
Ext4Unload (
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *DeviceHandleBuffer;
  UINTN       DeviceHandleCount;
  UINTN       Index;

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for(Index = 0; Index < DeviceHandleCount; Index++) {
    EFI_HANDLE  Handle;

    Handle = DeviceHandleBuffer[Index];

    Status = EfiTestManagedDevice (Handle, ImageHandle, &gEfiDiskIoProtocolGuid);

    if(Status == EFI_SUCCESS) {
      Status = gBS->DisconnectController (Handle, ImageHandle, NULL);

      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  FreePool (DeviceHandleBuffer);

  Status = EfiLibUninstallAllDriverProtocols2 (
             &gExt4BindingProtocol,
             &gExt4ComponentName,
             &gExt4ComponentName2,
             NULL,
             NULL,
             NULL,
             NULL
             );

  return Status;
}

EFI_STATUS EFIAPI
Ext4IsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL
  )
{
  // Note to self: EFI_OPEN_PROTOCOL_TEST_PROTOCOL lets us not close the
  // protocol and ignore the output argument entirely

  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  NULL,
                  BindingProtocol->ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if(EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  BindingProtocol->ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  return Status;
}

EFI_STATUS EFIAPI
Ext4Bind (
  IN EFI_DRIVER_BINDING_PROTOCOL *BindingProtocol,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH *RemainingDevicePath OPTIONAL
  )
{
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_DISK_IO2_PROTOCOL  *DiskIo2;
  EFI_BLOCK_IO_PROTOCOL  *blockIo;
  EFI_STATUS             Status;

  DiskIo2 = NULL;

  DEBUG ((EFI_D_INFO, "[Ext4] Binding to controller\n"));

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&DiskIo,
                  BindingProtocol->ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if(EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "[Ext4] Controller supports DISK_IO\n"));

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIo2ProtocolGuid,
                  (VOID **)&DiskIo2,
                  BindingProtocol->ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  // It's okay to not support DISK_IO2

  if(DiskIo2 != NULL) {
    DEBUG ((EFI_D_INFO, "[Ext4] Controller supports DISK_IO2\n"));
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&blockIo,
                  BindingProtocol->ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if(EFI_ERROR (Status)) {
    goto Error;
  }

  DEBUG ((EFI_D_INFO, "Opening partition\n"));

  Status = Ext4OpenPartition (ControllerHandle, DiskIo, DiskIo2, blockIo);

  if(!EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "[ext4] Error mounting %x\n", Status));

Error:
  if(DiskIo) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIoProtocolGuid,
           BindingProtocol->ImageHandle,
           ControllerHandle
           );
  }

  if(DiskIo2) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIo2ProtocolGuid,
           BindingProtocol->ImageHandle,
           ControllerHandle
           );
  }

  if(blockIo) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiBlockIoProtocolGuid,
           BindingProtocol->ImageHandle,
           ControllerHandle
           );
  }

  return Status;
}
