#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/FileHandleLib.h>
#include <Guid/FileInfo.h>

////////DEBUG/////////

#include <Library/DebugLib.h>


//EDK2 is C99 base.
////////EDK2 includes End////////

#include "./bin/SystemBinaryUtility.h"
#include "./bin/BootFileSystemUtility.h"

////////ZCP includes End////////

#define MAX_BUFFER_SIZE 256
#define TYPOLOCATION L">> "

//나중에 통합 BootService 쉘을 작성해서 모든 부트서비스 객체들을 붙일 것.

CHAR16 input_buffer[MAX_BUFFER_SIZE];
LIST_ENTRY *TokenArrayEntry = NULL;

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE imgHandle, IN EFI_SYSTEM_TABLE* sysTable)
{
    gST = sysTable;
    gBS = sysTable->BootServices;

    SBU shell;
    SBU_InitializeLib(&shell);  //attach functions in shell Object

    BFSU FSys;
    BFSU_InitializeLib(&FSys);

    DEBUG ((DEBUG_INFO, "Pointer Value: 0x%p\r\n", TokenArrayEntry));
    DEBUG ((DEBUG_INFO, "Pointer Variable Address: 0x%p\r\n", &TokenArrayEntry));

    Print(L"Initializing Success, Input Activate : \r\n");

    while (TRUE) {
        Print(TYPOLOCATION);
        shell.ReadLine(&shell, input_buffer, MAX_BUFFER_SIZE);

        if(!StrnCmp(input_buffer, L"reset ", 6)) {
            EFI_STATUS Status;
            DEBUG((DEBUG_INFO, "(Init) : ListHead TokenArrayEntry init\r\n"));
            
            Status = SBU_TokenHandler(&shell, input_buffer + 5, 1, &TokenArrayEntry);
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
            DEBUG((DEBUG_INFO, "(Init) : ListHead TokenArrayEntry init\r\n"));

            Status = SBU_TokenHandler(&shell, input_buffer + 4, 0, &TokenArrayEntry);

            if(EFI_ERROR(Status)) {
                DEBUG((DEBUG_WARN, "Token Parsing FAILED with code %d\r\n", Status));
                Token_List_Destructor(TokenArrayEntry);
            }
            else DEBUG((DEBUG_INFO, "Token Parsing SUCCESS\r\n"));   

            if(IsListEmpty(TokenArrayEntry)) {
                DEBUG((DEBUG_WARN, "!! TokenArrayEntry is Empty !!\r\n"));
            } else {
                CommandToken *TokenParsed;
                LIST_ENTRY *Link_Entered;

                for (Link_Entered = GetFirstNode (TokenArrayEntry); !IsNull (TokenArrayEntry, Link_Entered); 
                        Link_Entered = GetNextNode (TokenArrayEntry, Link_Entered)) {
                    TokenParsed = BASE_CR(Link_Entered, CommandToken, Link);
                    if(TokenParsed->Signature != C_Token_Signature) DEBUG((DEBUG_WARN, "(WARN) : Data Corrupted\r\n"));
                    DEBUG((DEBUG_INFO, "Parsed Data : %s  index : %d  type : %x\r\n", TokenParsed->Token, TokenParsed->TokenPosition, TokenParsed->TokenType));
                }
                TokenParsed = NULL;
                Link_Entered = NULL;
                DEBUG((DEBUG_INFO, "Token Print End, Delete List\r\n"));
            }
            Token_List_Destructor(TokenArrayEntry);
        }

        if(!StrCmp(input_buffer, L"Shutdown") || !StrCmp(input_buffer, L"shutdown")) {
            shell.ShutdownCommand(&shell);
            return EFI_SUCCESS;
        }

        if(!StrCmp(input_buffer, L"WhoamI") || !StrCmp(input_buffer, L"whoamI"))
            shell.WhoamI(&shell);
        
        if(!StrnCmp(input_buffer, L"cat ", 4)) {
            EFI_STATUS Status;
            Status = SBU_TokenHandler(&shell, input_buffer + 5, 1, &TokenArrayEntry);
            if(EFI_ERROR(Status)) {
                Print(L"Token Parsing FAILED with code %d\r\n", Status);
                Token_List_Destructor(TokenArrayEntry);
            }
            //init command, end parsing, all the value is in the TokenArrayEntry DRL-list

            /*
            다음 구조체를 선언 : 파일 이름(또는 경로); 파일 종단(확장자 여부) 확인; 옵션값
            이 구조체가 이 명령어가 무엇을 해야되는지 알려주는 것
            파일 이름은 파일의 경로(상대, EDK2의 SimpleFileProtocol이 자동으로 해줌)
            파일 종단 여부는 파일의 확장자가 있는지 검사(. 표시), 이후, 파일의 종류를 정함
            옵션값은 이 파일을 생성하는 것인지, 읽는 것인지, 쓰는 것인지 확인
            아직 쓰는 것은 만들어지지는 않았으니, 비활성 상태로 놔둠.
            

            struct {
                CHAR16 * FileName;
                BOOLEAN FileType;
                BOOLEAN create;
                BOOLEAN read;
                BOOLEAN write;
                CHAR16 * Text;
            } catCMDresult;*/


        }
    }

    return EFI_SUCCESS;
}
