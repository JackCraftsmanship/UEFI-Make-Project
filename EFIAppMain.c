#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/FileHandleLib.h>
#include <Guid/FileInfo.h>


//EDK2 is C99 base.
////////EDK2 includes End////////

#include "./bin/SystemBinaryUtility.h"
#include "./bin/BootFileSystemUtility.h"

////////ZCP includes End////////

#define MAX_BUFFER_SIZE 256
#define TYPOLOCATION L">> "

//나중에 통합 BootService 쉘을 작성해서 모든 부트서비스 객체들을 붙일 것.

CHAR16 input_buffer[MAX_BUFFER_SIZE];
LIST_ENTRY *TokenArrayEntry;

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
        Print(TYPOLOCATION);
        shell.ReadLine(&shell, input_buffer, MAX_BUFFER_SIZE);

        if(!StrnCmp(input_buffer, L"reset", 5)) {
            EFI_STATUS Status;
            InitializeListHead(TokenArrayEntry);
            
            Status = SBU_TokenHandler(&shell, input_buffer + 5, 1, TokenArrayEntry);
            if(EFI_ERROR(Status)) {
                Print(L"Token Parsing FAILED with code %d\r\n", Status);
                Token_List_Destructor(TokenArrayEntry);
            }
            CommandToken *TokenParsed;
            LIST_ENTRY *Link_Entered;

            for (Link_Entered = GetFirstNode (TokenArrayEntry); !IsNull (TokenArrayEntry, Link_Entered); 
                    Link_Entered = GetNextNode (TokenArrayEntry, Link_Entered)) {
                TokenParsed = BASE_CR(Link_Entered, CommandToken, Link);
                shell.RebootCommand(&shell, TokenParsed->Token);
            }
            TokenParsed = NULL;
            Link_Entered = NULL;
            Token_List_Destructor(TokenArrayEntry);
        }

        if(!StrnCmp(input_buffer, L"test", 4)) {
            EFI_STATUS Status;
            InitializeListHead(TokenArrayEntry);

            Status = SBU_TokenHandler(&shell, input_buffer + 4, 0, TokenArrayEntry);
            Print(L"\r\n");
            if(EFI_ERROR(Status)) {
                Print(L"Token Parsing FAILED with code %d\r\n", Status);
                Token_List_Destructor(TokenArrayEntry);
            }
            else Print(L"Token Parsing SUCCESS\r\n");   

            if(IsListEmpty(TokenArrayEntry)) {
                Print(L"!! TokenArrayEntry is Empty !!\r\n");
            } else {
                Print(L"Show : \r\n");
                CommandToken *TokenParsed;
                LIST_ENTRY *Link_Entered;

                for (Link_Entered = GetFirstNode (TokenArrayEntry); !IsNull (TokenArrayEntry, Link_Entered); 
                        Link_Entered = GetNextNode (TokenArrayEntry, Link_Entered)) {
                    TokenParsed = BASE_CR(Link_Entered, CommandToken, Link);
                    Print(L"Parsed Data : %s  ", TokenParsed->Token);
                    Print(L"index : %d  ", TokenParsed->TokenPosition);
                    Print(L"type : %x\r\n", TokenParsed->TokenType);
                }
                TokenParsed = NULL;
                Link_Entered = NULL;
            }
            Token_List_Destructor(TokenArrayEntry);
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
                StrCpyS(TempFileNameContainer, MAX_BUFFER_SIZE, (input_buffer + 4));
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
