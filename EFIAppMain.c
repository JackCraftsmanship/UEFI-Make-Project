#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/BaseLib.h>

//EDK2 is C99 base.
////////EDK2 includes End////////

#include "./bin/StringHandle.h"

////////ZCP includes End////////

#define MAX_BUFFER_SIZE 256
#define TYPOLOCATION L">> "

/**This Object is renamed identifier of _System_Binary_Utility.
 * It will give the interface of Basic Termianl Control in Boot Service
 * @note when init, attach with this function : SBU_InitializeLib
 */
typedef struct _System_Binary_Utility SBU;

struct _System_Binary_Utility {

    /**This is Part of _System_Binary_Utility or SBU, 
     * Read a Line and save into System Text Buffer.
     * @param This self
     * @param buffer System Text Buffer
     * @param BufferSize Max Buffer Size of System
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*ReadLine)(
        IN SBU *This,
        OUT CHAR16 *Buffer,
        IN UINTN BufferSize
    );

    /**This is Part of _System_Binary_Utility or SBU, 
     * Call Reboot Command with Option.
     * @note This will be change from select-one-at-a-time to Option parsing
     * @param This self
     * @param Option option of Reboot command. See EFI Spec docs
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*RebootCommand)(
        IN SBU *This,
        IN CHAR16 *Option
    );

    /**This is Part of _System_Binary_Utility or SBU, 
     * Call shutdown Command.
     * @note This will be add some Options
     * @param This self
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*ShutdownCommand)(
        IN SBU *This
    );

    /**This is Part of _System_Binary_Utility or SBU, 
     * Internal Option Handler for handle Option in command.
     * @note Currently not vaild
     * @param This self
     * @param SourceString Source string that want to parsing option
     * @param OptionIdentifier Option Identifier
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*OptionHandler)(
        IN SBU *This,
        IN CHAR16 *SourceString,
        IN CHAR16 *OptionIdentifier
    );

    /**This is Part of _System_Binary_Utility or SBU, 
     * Who Am I String Printer. Suitable for testing BootService.
     * @param This self
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*WhoamI)(
        IN SBU *This
    );
};

EFI_STATUS SBU_Readline(IN SBU *This, OUT CHAR16 *Buffer, IN UINTN BufferSize) {
    EFI_STATUS Status;
    EFI_INPUT_KEY CharTemp;
    UINTN index;

    //cursor control variable
    UINTN Pos = 0;      //End of Line in Buffer
    UINTN cursor = 0;   //Current Cursor Position
    UINTN base_row = gST->ConOut->Mode->CursorRow;  //base Row Position
    UINTN base_col = gST->ConOut->Mode->CursorColumn;   //base Column Position


    while(1){
        //using BootService, EventListener
        gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &index);

        //check ReadError
        Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &CharTemp);
        
        //pass when Error
        if(EFI_ERROR(Status)) continue;

        //move Cursor L ro R
        if(CharTemp.UnicodeChar == 0) {
            if(CharTemp.ScanCode == SCAN_LEFT && cursor > 0) {
                cursor--;
                gST->ConOut->SetCursorPosition(gST->ConOut, base_col + cursor, base_row);
            }
            if(CharTemp.ScanCode == SCAN_RIGHT && cursor < Pos) {
                cursor++;
                gST->ConOut->SetCursorPosition(gST->ConOut, base_col + cursor, base_row);
            }
            continue;
        }

        //other exception code handle & normal typing
        if(CharTemp.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            Buffer[Pos] = L'\0';
            Print(L"\r\n");
            return EFI_SUCCESS;
        } else if (CharTemp.UnicodeChar == CHAR_BACKSPACE) {
            if(cursor > 0 && Pos > 0) {
                cursor--;
                Pos--;
                Print(L"\b \b");
                if(cursor != Pos) {
                    for(int i = cursor; i < Pos; i++) {
                    Buffer[i] = Buffer[i + 1];
                    Print(L"%c", Buffer[i]);
                    }
                    Print(L" ");
                    gST->ConOut->SetCursorPosition(gST->ConOut, base_col + cursor, base_row);
                }
                Buffer[Pos] = L'\0';
            }
        } else if(CharTemp.UnicodeChar == 0x15) { //ctrl + U
            while (Pos > 0) {
                Pos--;
                Print(L"\b \b");
            }
            cursor = 0;
            Buffer[0] = L'\0';
        } else if(Pos < BufferSize - 1) {
            if(cursor != Pos) {
                for(int i = Pos; i > cursor; i--) {
                Buffer[i] = Buffer[i - 1];
                }
            }
            Buffer[cursor++] = CharTemp.UnicodeChar;
            Buffer[++Pos] = L'\0';
            Print(L"%c", CharTemp.UnicodeChar);
            if(cursor != Pos) {
                for(int i = cursor; i < Pos; i++) {
                Print(L"%c", Buffer[i]);
                }
                gST->ConOut->SetCursorPosition(gST->ConOut, base_col + cursor, base_row);
            }
        }

    }
    return EFI_SUCCESS;
}

EFI_STATUS SBU_ReBoot(IN SBU *This, IN CHAR16 *Option) {
    EFI_RESET_TYPE resetType;

    if(!StrCmp(Option, L"ColdReset")) {
        Print(L"Reset with device shutdown.");
        resetType = EfiResetCold;
    }
    else if(!StrCmp(Option, L"NormalReset")) {
        Print(L"Reset without device shutdown");
        resetType = EfiResetWarm;
    } else {
        Print(L"Enter Correct ResetType : ColdReset or Reset\r\n");
        return EFI_SUCCESS;
    }
    
    gST->RuntimeServices->ResetSystem(resetType, EFI_SUCCESS, 0, NULL);
    return EFI_SUCCESS;
}

EFI_STATUS SBU_Shutdown(IN SBU *This) {
    gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return EFI_SUCCESS;
}

EFI_STATUS SBU_OptionHandler(IN SBU *This, IN CHAR16 *SourceString, IN CHAR16 *OptionIdentifier) {
    return EFI_SUCCESS;
}

EFI_STATUS SBU_WhoamI(IN SBU *This) {
    Print(L"This shell is part of Custom EFI Boot Service\r\nMade by Jack::ZeroCP\r\n");
    return EFI_SUCCESS;
}

EFI_STATUS SBU_InitializeLib(IN SBU *This)
{
    This->ReadLine = SBU_Readline;
    This->RebootCommand = SBU_ReBoot;
    This->ShutdownCommand = SBU_Shutdown;
    This->WhoamI = SBU_WhoamI;
    This->OptionHandler = SBU_OptionHandler; 

    return EFI_SUCCESS;
}

/**This Object is renamed identifier of _Boot_File_System_Utility.
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
