#include "./BootFileSystemUtility.h"

///////////////////////////Inline Method//////////////////////////////

EFI_STATUS BFSU_ProtocolHeader(BFSU *This, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **FsProtocol, EFI_FILE_PROTOCOL **RootHandle, EFI_STATUS *Status) {
    *Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)FsProtocol);
    
    if (EFI_ERROR(*Status)) {
        Print(L"Cannot find Protocol.\r\n");
        return *Status;
    }

    *Status = (*FsProtocol)->OpenVolume((*FsProtocol), RootHandle);

    if (EFI_ERROR(*Status)) {
        Print(L"Unknown Error occur during open the volume.\r\n");
        return *Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS BFSU_FileNameChecker(BFSU *This, CHAR16 *FileName, UINTN FileNameLength) {
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
            case L'/':
            case L'\\':
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
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FsProtocol;    //FileSystemProtocol
    EFI_FILE_PROTOCOL *RootHandle = NULL;  //root directory handle
    EFI_STATUS Status;

    BFSU_ProtocolHeader(This, &FsProtocol, &RootHandle, &Status);
    
    EFI_FILE_PROTOCOL *FileHandle = NULL;  //current file handle
    Status = RootHandle->Open(RootHandle, &FileHandle, FileName, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(Status)) {
        Print(L"Cannot Create File : ");
        if(Status == EFI_NO_MEDIA) {
            Print(L"NO MEDIA\r\n");
        }
        else if(Status == EFI_VOLUME_CORRUPTED) {
            Print(L"VOLUME CORRUPTED\r\n");
        }
        else if(Status == EFI_DEVICE_ERROR) {
            Print(L"DEVICE ERROR\r\n");
        }
        else if(Status == EFI_ACCESS_DENIED) {
            Print(L"ACCESS DENIED\r\n");
        }
        else if(Status == EFI_VOLUME_FULL) {
            Print(L"VOLUME FULL\r\n");
        }
        else if(Status == EFI_OUT_OF_RESOURCES) {
            Print(L"OUT OF RESOURCE\r\n");
        }
        else if(Status == EFI_UNSUPPORTED) {
            Print(L"UNSUPPORT\r\n");
        }
        else if(Status == EFI_INVALID_PARAMETER) {
            Print(L"INVAILD_PARAMETER\r\n");
        }
        RootHandle->Close(RootHandle);
        return Status;
    }

    Print(L"Make File %s in %s\r\n", FileName, This->CurrentDirectoryPath);
    FileHandle->Close(FileHandle);
    RootHandle->Close(RootHandle);
    return EFI_SUCCESS;
}

////////////////////////directory handler///////////////////////////

EFI_STATUS BFSU_GotoDirectory(BFSU *This, CHAR16 *DirectoryPath);

EFI_STATUS BFSU_InitializeLib(BFSU *This) {
    This->CurrentDirectoryPath = L"~";
    This->ProtocolHeader = BFSU_ProtocolHeader;
    This->FileNameCheck = BFSU_FileNameChecker;
    This->MakeFile = BFSU_MakeFile;

    return EFI_SUCCESS;
}
