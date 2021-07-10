/**
 * @file Driver entry point
 *
 * Copyright (c) 2021 Pedro Falcato All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "Ext4.h"

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

extern EFI_COMPONENT_NAME_PROTOCOL  gExt4ComponentName;

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
  // TODO: Implement
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Ext4Unload (
  IN EFI_HANDLE ImageHandle
  )
{
  // TODO: Implement
  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL  gExt4BindingProtocol =
{
  Ext4IsBindingSupported,
  Ext4Bind,
  Ext4Stop,
  EXT4_DRIVER_VERSION
};

EFI_STATUS
EFIAPI
Ext4EntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  st = EfiLibInstallAllDriverProtocols2 (
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

  if(EFI_ERROR (st)) {
    return st;
  }

  return Ext4InitialiseUnicodeCollation (ImageHandle);
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

  EFI_STATUS  st = gBS->OpenProtocol (
                          ControllerHandle,
                          &gEfiDiskIoProtocolGuid,
                          NULL,
                          BindingProtocol->ImageHandle,
                          ControllerHandle,
                          EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                          );

  if(EFI_ERROR (st)) {
    return st;
  }

  st = gBS->OpenProtocol (
              ControllerHandle,
              &gEfiBlockIoProtocolGuid,
              NULL,
              BindingProtocol->ImageHandle,
              ControllerHandle,
              EFI_OPEN_PROTOCOL_TEST_PROTOCOL
              );
  return st;
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

  DiskIo2 = NULL;

  DEBUG ((EFI_D_INFO, "[Ext4] Binding to controller\n"));

  EFI_STATUS  st = gBS->OpenProtocol (
                          ControllerHandle,
                          &gEfiDiskIoProtocolGuid,
                          (VOID **)&DiskIo,
                          BindingProtocol->ImageHandle,
                          ControllerHandle,
                          EFI_OPEN_PROTOCOL_BY_DRIVER
                          );

  if(EFI_ERROR (st)) {
    return st;
  }

  DEBUG ((EFI_D_INFO, "[Ext4] Controller supports DISK_IO\n"));

  st = gBS->OpenProtocol (
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

  st = gBS->OpenProtocol (
              ControllerHandle,
              &gEfiBlockIoProtocolGuid,
              (VOID **)&blockIo,
              BindingProtocol->ImageHandle,
              ControllerHandle,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
              );

  if(EFI_ERROR (st)) {
    goto error;
  }

  DEBUG ((EFI_D_INFO, "Opening partition\n"));

  st = Ext4OpenPartition (ControllerHandle, DiskIo, DiskIo2, blockIo);

  if(!EFI_ERROR (st)) {
    return st;
  }

  DEBUG ((EFI_D_INFO, "[ext4] Error mounting %x\n", st));

error:
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

  return st;
}
