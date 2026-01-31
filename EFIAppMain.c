#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/BaseLib.h>

//EDK2 is C99 base.
////////EDK2 includes End////////

#include "./bin/SystemBinaryUtility.h"
#include "./bin/BootFileSystemUtility.h"

////////ZCP includes End////////

#define MAX_BUFFER_SIZE 256
#define TYPOLOCATION L">> "

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

        /*
        if(!StrnCmp(input_buffer, L"reset", 5)) {
            static OptionFlag ResetOption[] = {
                {L"-c", 2},
                {L"-w", 2},
                {L"--cold", 6},
                {L"--warm", 6}
            };
            static OptionContainer ResetOptionContainer = {ResetOption, 7, 1};
            EFI_STATUS Status;
            CHAR16 OptionArray[ResetOptionContainer.MaxInputArrayLength][ResetOptionContainer.MaxInputOptionLength];

            Status = shell.OptionHandler(&shell, input_buffer, ResetOptionContainer.OptionArray, 4,
                                ResetOptionContainer.MaxInputOptionLength, ResetOptionContainer.MaxInputArrayLength, OptionArray);
            if(EFI_ERROR(Status)) {
                if(Status != EFI_NOT_FOUND) {
                    Print(L"An Error Occure During Find Option\r\n");
                    goto RESET_END;
                }
            }

            if(OptionArray[0] == NULL) {
                Print(L"\'reset\' need option, type \'reset help\' for more info\r\n");
                goto RESET_END;
            }

            Print(L"final Output : %s\r\n", OptionArray[0]);

            shell.RebootCommand(&shell, OptionArray[0]);
            RESET_END:
        }*/

        if(!StrnCmp(input_buffer, L"test", 4)) {
            CommandToken Token1;
            EFI_STATUS Status;
            Status = Token_OptionHandler(input_buffer + 4, &Token1);
            if(EFI_ERROR(Status)) {
                Print(L"Token Parsing fail with %d\r\n", Status);
            } else {
                Print(L"Token input : %s\r\n", Token1.Token);
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
                StrCatS(TempFileNameContainer, MAX_BUFFER_SIZE, (input_buffer + 4));
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
