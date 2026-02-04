#include "./BootFileSystemUtility.h"

///////////////////////////Inline Method//////////////////////////////

EFI_STATUS BFSU_ProtocolHeader(IN BFSU *This, OUT EFI_FILE_PROTOCOL **RootHandle) {
    EFI_STATUS Status;
    Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID*)&This->FsProtocol);
    
    if (EFI_ERROR(Status)) {
        Print(L"Cannot find Protocol.\r\n");
        return Status;
    }

    Status = This->FsProtocol->OpenVolume((This->FsProtocol), RootHandle);

    if (EFI_ERROR(Status)) {
        Print(L"Unknown Error occur during open the volume.\r\n");
        return Status;
    }

    Status = This->RootHandle->Open(This->RootHandle, &This->SavedPath.handler, L".", EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
    if (EFI_ERROR(Status)) return Status;

    return EFI_SUCCESS;
}

EFI_STATUS BFSU_FileNameChecker(IN BFSU *This, IN CHAR16 *FileName, IN UINTN FileNameLength) {
    if(FileNameLength > 256) return FILE_CHECK_TOO_LONG;
    if(FileName[0] == L'\0') return FILE_CHECK_VOID;
    if(FileName[FileNameLength - 1] == L'.') return FILE_CHECK_INVAILD_NAME;

    for(UINTN i = 0; i < FileNameLength; i++) {
        if(FileName[i] == L'\0') break;
        else if((UINTN)FileName[i] > 0 && (UINTN)FileName[i] < 32) {
            return FILE_CHECK_INVAILD_NAME;
        }
        switch ((UINTN)FileName[i]) {
            case L'<':
            case L'>':
            case L':':
            case L'\"':
            //case L'/':
            //case L'\\':
            case L'|':
            case L'?':
            case L'*':
                return FILE_CHECK_INVAILD_NAME;
                break;
        }
    }
    return FILE_CHECK_NORMAL;
}

///////////////////////////file handler//////////////////////////////

EFI_STATUS BFSU_MakeFile(IN BFSU *This, IN CHAR16 *FileName) {
    EFI_STATUS Status;
    
    Status = This->SavedPath.handler->Open(This->SavedPath.handler, &This->CurrentPath.handler, FileName,
                                    EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(Status)) return Status;

    Print(L"Make File %s in %s\r\n", FileName, This->SavedPath.absolute_path);
    This->CurrentPath.handler->Close(This->CurrentPath.handler);
    return EFI_SUCCESS;
}

EFI_STATUS BFSU_ReadFile(IN BFSU *This, IN CHAR16 *FileName) {
    EFI_STATUS Status;
    
    Status = This->SavedPath.handler->Open(This->SavedPath.handler, &This->CurrentPath.handler, FileName, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(Status)) return Status;

    Print(L"Open File %s in %s\r\n", FileName, This->SavedPath.absolute_path);

    EFI_FILE_INFO *FileInfo;
    UINTN         BufferSize;

    Status = This->CurrentPath.handler->GetInfo(This->CurrentPath.handler, &gEfiFileInfoGuid, &BufferSize, NULL);

    if (Status == EFI_BUFFER_TOO_SMALL) {
        FileInfo = AllocateZeroPool(BufferSize);
        Status = This->CurrentPath.handler->GetInfo(This->CurrentPath.handler, &gEfiFileInfoGuid, &BufferSize, FileInfo);
    }
    This->CurrentFile.file_info = FileInfo;
    This->CurrentFile.DataSize = This->CurrentFile.file_info->FileSize;
    This->CurrentFile.Data = AllocateZeroPool(This->CurrentFile.DataSize);

    Print(L"%s\r\n", This->CurrentPath.handler->Read(This->CurrentPath.handler, &This->CurrentFile.DataSize, This->CurrentFile.Data));

    This->CurrentPath.handler->Close(This->CurrentPath.handler);
    //임시 종료구간
    FreePool(This->CurrentFile.file_info);
    FreePool(This->CurrentFile.Data);
    This->CurrentFile.DataSize = 0;
    This->CurrentFile.absolute_path = L'\0';

    return EFI_SUCCESS;
}

EFI_STATUS BFSU_WriteBackFile(IN BFSU *This, IN CHAR16 *FileName, IN CHAR16 *StringToWriteBack) {
    EFI_STATUS Status;
    
    Status = This->SavedPath.handler->Open(This->SavedPath.handler, &This->CurrentPath.handler, FileName, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(Status)) return Status;

    Print(L"Open Writtable File %s in %s\r\n", FileName, This->CurrentPath.absolute_path);

    //do something

    This->CurrentPath.handler->Close(This->CurrentPath.handler);
    return EFI_SUCCESS;
}

////////////////////////directory handler///////////////////////////

EFI_STATUS BFSU_MakeDirectory(IN BFSU *This, IN CHAR16 *DirectoryPath) {
    EFI_STATUS Status;

    This->CurrentPath.handler = NULL;
    Status = This->SavedPath.handler->Open(This->SavedPath.handler, &This->CurrentPath.handler, DirectoryPath,
                                    EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);

    if (EFI_ERROR(Status)) return Status;

    Print(L"Make Directory in the Directory : %s\r\n", This->SavedPath.absolute_path);
    This->CurrentPath.handler->Close(This->CurrentPath.handler);
    return EFI_SUCCESS;
}

EFI_STATUS BFSU_GotoDirectory(IN BFSU *This, IN CHAR16 *DirectoryPath) {
    EFI_STATUS Status;
    CHAR16 *CurrentPath_Str;

    This->CurrentPath.handler = NULL;
    Status = This->SavedPath.handler->Open(This->SavedPath.handler, &This->CurrentPath.handler, DirectoryPath,
                                    EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);

    Status = FileHandleGetFileName(This->CurrentPath.handler, &CurrentPath_Str);
    if (EFI_ERROR(Status)) return Status;

    if(This->CurrentPath.absolute_path == NULL) FreePool(This->CurrentPath.absolute_path);
    This->CurrentPath.absolute_path = CurrentPath_Str;

    Print(L"Currently in the Directory : %s\r\n", This->CurrentPath.absolute_path);
    This->SavedPath.handler->Close(This->SavedPath.handler);
    This->SavedPath.handler = This->CurrentPath.handler;
    This->CurrentPath.handler = NULL;
    return EFI_SUCCESS;
}

EFI_STATUS BFSU_InitializeLib(IN BFSU *This) {
    EFI_STATUS Status;

    This->CurrentPath.handler = NULL;
    This->CurrentPath.absolute_path = L"\\";
    This->SavedPath.handler = NULL;
    This->SavedPath.absolute_path = L"\\";

    This->ProtocolHeader = BFSU_ProtocolHeader;
    This->FileNameCheck = BFSU_FileNameChecker;
    This->MakeFile = BFSU_MakeFile;
    This->OpenFile = BFSU_ReadFile;
    This->WriteBackFile = BFSU_WriteBackFile;
    This->MakeDirectory = BFSU_MakeDirectory;
    This->GotoDirectory = BFSU_GotoDirectory;

    Status = BFSU_ProtocolHeader(This, &This->RootHandle);
    if(EFI_ERROR(Status)) return Status;

    return EFI_SUCCESS;
}
