#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/BaseLib.h>

//EDK2 is C99 base.
////////EDK2 includes End////////

#include "./bin/StringHandle.h"
#include "./bin/SystemBinaryUtility.h"

////////ZCP includes End////////

#define MAX_BUFFER_SIZE 256
#define TYPOLOCATION L">> "

/** This Object is renamed identifier of _Boot_File_System_Utility.
 * It will give the interface of File System via EFI Simple File System Protocol
 * @note when init, attach with this function : BFSU_InitializeLib
 */
typedef struct _Boot_File_System_Utility BFSU;

struct _Boot_File_System_Utility {

    CHAR16 *CurrentDirectoryPath;

    EFI_STATUS (*ProtocolHeader)(
        BFSU *This,
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **FsProtocol,
        EFI_FILE_PROTOCOL **RootHandle,
        EFI_STATUS *Status
    );
    EFI_STATUS (*FileNameCheck)(
        BFSU *This,
        CHAR16 *FileName,
        UINTN FileNameLength
    );

    EFI_STATUS (*MakeFile)(
        BFSU *This,
        CHAR16 *FileName
    );
    EFI_STATUS (*FileOpen)(
        BFSU *This,
        CHAR16 *FilePath
    );
    EFI_STATUS (*SimpleFileWriteBack)(
        BFSU *This,
        CHAR16 *FilePath,
        CHAR16 *StringToWriteBack
    );

    EFI_STATUS (*MakeDirectory)(
        BFSU *This,
        CHAR16 *DirectoryName
    );
    EFI_STATUS (*GotoDirectory)(
        BFSU *This,
        CHAR16 *DirectoryPath
    );
};

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

// fileName Handle Flags
#define FILE_CHECK_NORMAL 0x00
#define FILE_CHECK_VOID 0x01
#define FILE_CHECK_INVAILD_NAME 0x02
#define FILE_CHECK_TOO_LONG 0x04

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

//나중에 통합 BootService 쉘을 작성해서 모든 부트서비스 객체들을 붙일 것.

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE imgHandle, IN EFI_SYSTEM_TABLE* sysTable)
{
    gST = sysTable;
    gBS = sysTable->BootServices;

    SBU shell;
    SBU_InitializeLib(&shell);  //attach functions in shell Object

    BFSU FSys;
    BFSU_InitializeLib(&FSys);

    Print(L"Initializing Success, Input Activate : \r\n");

    while (TRUE) {
        CHAR16 input_buffer[MAX_BUFFER_SIZE];
        Print(TYPOLOCATION);
        shell.ReadLine(&shell, input_buffer, MAX_BUFFER_SIZE);

        if(!StrCmp(input_buffer, L"Reset") || !StrCmp(input_buffer, L"reset")) {
            Print(L"Select Reset Type : \r\n1. ColdReset\r\n2. NormalReset\r\n3. exit\r\n");
            while (TRUE) {
                Print(TYPOLOCATION);
                shell.ReadLine(&shell, input_buffer, MAX_BUFFER_SIZE);
                if(!StrCmp(input_buffer, L"exit")) {
                    input_buffer[0] = L'\0';
                    break;
                } else shell.RebootCommand(&shell, input_buffer);
            }
        }

        if(!StrCmp(input_buffer, L"Shutdown") || !StrCmp(input_buffer, L"shutdown")) {
            shell.ShutdownCommand(&shell);
            return EFI_SUCCESS;
        }

        if(!StrCmp(input_buffer, L"WhoamI") || !StrCmp(input_buffer, L"whoamI"))
            shell.WhoamI(&shell);
        
        if(!StrnCmp(input_buffer, L"cat", 3)) {
            if(input_buffer[3] == 0x20) {       //0x20 == space
                CHAR16 TempFileNameContainer[256];
                EFI_STATUS Status;
                Status = SubStr(input_buffer, TempFileNameContainer, 4);
                if(EFI_ERROR(Status)) {
                    Print(L"Argument Error While reading the Name.\r\n");
                    goto CAT_END;
                }
                Status = FSys.FileNameCheck(&FSys, TempFileNameContainer, StrLen(TempFileNameContainer));
                if(Status != 0x00) {
                    Print(L"Invaild Name, Please Try with Different Name.\r\n");
                    goto CAT_END;
                }
                FSys.MakeFile(&FSys, TempFileNameContainer);
            }
            CAT_END:
        }
    }

    return EFI_SUCCESS;
}
