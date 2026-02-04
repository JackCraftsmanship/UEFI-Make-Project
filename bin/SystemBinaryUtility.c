#include "./SystemBinaryUtility.h"

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

    if(!StrCmp(Option, L"-c") || !StrCmp(Option, L"--cold")) {
        Print(L"Reset with device shutdown.");
        resetType = EfiResetCold;
    }
    else if(!StrCmp(Option, L"-w") || !StrCmp(Option, L"--warm")) {
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


/*
토큰을 사용자 지정 연결 리스트 객체로 만들어서 관리하는 것이 적합.
그렇기에 연결 리스트를 위한 별도의 파일을 만들 것. 
 -> EDK2 제공 이중 환형 연결 리스트 사용할 것임.
*/

EFI_STATUS SBU_TokenHandler(IN SBU *This, IN CHAR16 *SourceBuffer, IN UINTN TokenMaxAmount, IN OUT LIST_ENTRY *TokenArrayPointer) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;
    //if(TokenArrayPointer != NULL) return RETURN_ACCESS_DENIED;      //can occure Dangling pointer problem

    UINTN StrFront = 0;
    UINTN CharLength = 0;
    UINTN index = 1;
    UINTN StrLength = StrLen(SourceBuffer) + 1;

    LIST_ENTRY *ListInitHead;
    InitializeListHead(ListInitHead);

    CommandToken *TokenNode;
    EFI_STATUS Status;

    for(; StrFront < StrLength; StrFront++) {
        //add TokenNode Pool to heap
        TokenNode = AllocateZeroPool(sizeof(CommandToken));
        if(TokenNode == NULL) return RETURN_LOAD_ERROR;

        //CK string
        if(SourceBuffer[StrFront] == L'\0') return RETURN_SUCCESS;
        if(SourceBuffer[StrFront] == L' ') {
            if (SourceBuffer[StrFront + 1] == L'\0') return RETURN_SUCCESS;
            if (SourceBuffer[StrFront + 1] == L'=') goto SKIP_FOR_1;
            StrFront++;     //skip L' '
            if(SourceBuffer[StrFront] == L'-') {
                Print(L"Token Parsing start : index = %d\r\n", index);
                Status = Token_OptionHandler(SourceBuffer + StrFront, TokenNode, &CharLength);
                if(EFI_ERROR(Status)) goto TOKENHANDLER_FAILSAFE;
            } else if(SourceBuffer[StrFront] != L' ') {
                Print(L"Token Parsing start : index = %d\r\n", index);
                Status = Token_ArgumentHandler(SourceBuffer + StrFront, TokenNode, &CharLength, TOKENTYPE_ARGUMENT);
                if(EFI_ERROR(Status)) goto TOKENHANDLER_FAILSAFE;
            }
            StrFront += CharLength;
            Print(L"Parsing End with : %d\r\n Next Char : \'%c\'\r\n", StrFront, SourceBuffer[StrFront]);
            StrFront--;
            TokenNode->TokenPosition = index;
            TokenNode->Signature = C_Token_Signature;

            InsertTailList(ListInitHead, &TokenNode->Link);

            index++;
        } else if(SourceBuffer[StrFront] == L'=') {
            SKIP_FOR_1:
            if(index <= 1) return RETURN_INVALID_PARAMETER;
            if(SourceBuffer[StrFront] == L'-') return RETURN_INVALID_PARAMETER;
            Print(L"Addition Token Parsing start : index = %d\r\n", index);
            Status = Token_ArgumentHandler(SourceBuffer + StrFront, TokenNode, &CharLength, TOKENTYPE_ADDITION_ARGUMENT);
            if(EFI_ERROR(Status)) goto TOKENHANDLER_FAILSAFE;

            StrFront += CharLength;
            Print(L"Parsing End with : %d\r\n Next Char : \'%c\'\r\n", StrFront, SourceBuffer[StrFront]);
            StrFront--;
            TokenNode->TokenPosition = index;
            TokenNode->Signature = C_Token_Signature;

            InsertTailList(ListInitHead, &TokenNode->Link);

            index++;
        }
        else if(TokenMaxAmount != 0 && index >= TokenMaxAmount) return RETURN_SUCCESS;
        else return RETURN_INVALID_PARAMETER;
    }

    TokenArrayPointer = ListInitHead;
    ListInitHead = NULL;
    return RETURN_SUCCESS;

TOKENHANDLER_FAILSAFE:
    Print(L"Error Occured... Activate FailSafe : Delete All\r\n");
    Token_List_Destructor(ListInitHead);
    if(TokenNode != NULL) {
        if (TokenNode->Token != NULL) {
            FreePool (TokenNode->Token);
        }

        if (TokenNode->TokenKey != NULL) {
            FreePool (TokenNode->TokenKey);
        }
    }
    return Status;
}

VOID Token_List_Destructor(IN LIST_ENTRY *ListEntryPointer) {
   
    if (ListEntryPointer == NULL) return;
    LIST_ENTRY          *CurrentLink;
    LIST_ENTRY          *NextLink;
    CommandToken        *Entry;

    CurrentLink = GetFirstNode (ListEntryPointer);
        
    while (!IsNull (ListEntryPointer, CurrentLink)) {

        //get next and delete all inside
        NextLink = GetNextNode (ListEntryPointer, CurrentLink);
        Entry = BASE_CR (CurrentLink, CommandToken ,Link);

        if (Entry->Token != NULL) {
        FreePool (Entry->Token);
        }

        if (Entry->TokenKey != NULL) {
        FreePool (Entry->TokenKey);
        }

        RemoveEntryList (CurrentLink);
        FreePool (Entry);

        CurrentLink = NextLink;
    }
    return;
}

EFI_STATUS Token_ArgumentHandler(IN CHAR16 *SourceBuffer, IN OUT CommandToken *Token, OUT UINTN *Next, IN UINTN ArgumentType) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;

    UINT8 TypeOfStart = 0;        //other == 0, L'\"' == 1, L'=' == 2, when start with L'=' and contain L'\"' == 3
    UINTN StrBack = 0;
    UINTN StrFront = 0;
    UINTN StrLength = StrLen(SourceBuffer) + 1;
    CHAR16 *TempString;
    UINTN StringLength;
    EFI_STATUS Status;

    if(SourceBuffer[StrBack] == L'=') {
        StrFront++;
        StrBack++;
        if(SourceBuffer[StrBack] == L'\0') return RETURN_INVALID_PARAMETER;
        TypeOfStart |= 2;
    }

    if(SourceBuffer[StrBack] == L'\"') {
        StrFront++;
        StrBack++;
        if(SourceBuffer[StrBack] == L'\0') return RETURN_INVALID_PARAMETER;
        TypeOfStart |= 1;
        for(;StrBack < StrLength; StrBack++) {
            if(SourceBuffer[StrBack] == L'\"') {
                StringLength = StrBack - StrFront + 1;      //for NOT COPY end character : L'\"'
                break;
            }
        }
    } else {
        for(;StrBack < StrLength; StrBack++) {
            if(SourceBuffer[StrBack] == L' ' || SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L'=') {
                StringLength = StrBack - StrFront + 1;
                break;
            }
        }
    }

    TempString = AllocateZeroPool(StringLength);
    if(TempString == NULL) return RETURN_LOAD_ERROR;

    Status = StrnCpyS(TempString, StringLength, SourceBuffer + StrFront, StringLength - 1);
    if(EFI_ERROR(Status)) {
        FreePool(TempString);
        return Status;
    }
    Token->Token = TempString;
    TempString = NULL;

    if(TypeOfStart == 1) StrBack += 2;    //for two L'\"'
    else if(TypeOfStart == 2) StrBack += 1;     //for one L'='
    else if(TypeOfStart == 3) StrBack += 3;     //for one L'=' and two L'\"'

    Token->TokenKey = L"0000\0";     //init data, but just for first.
    Token->TokenType = ArgumentType;
    Token->TokenPosition = 0;

    *Next = StrBack - StrFront;
    return RETURN_SUCCESS;
}

EFI_STATUS Token_OptionHandler(IN CHAR16 *SourceBuffer, IN OUT CommandToken *Token, OUT UINTN *Next) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;

    UINTN TypeOfStart = FALSE;      //L"--" == TRUE, L'-' == FALSE
    UINTN StrBack = 0;
    UINTN StrFront = 0;
    UINTN StrLength = StrLen(SourceBuffer) + 1;
    CHAR16 *TempString;
    UINTN StringLength;
    EFI_STATUS Status;

    if(!StrnCmp(SourceBuffer + StrFront, L"--", 2)) {
        TypeOfStart = TRUE;
        StrFront += 2;
    } else StrFront += 1;

    StrBack = StrFront;
    if(SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L' ') return RETURN_INVALID_PARAMETER;
    StrBack++;

    for(;StrBack < StrLength; StrBack++) {
        if(SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L' ' || SourceBuffer[StrBack] == L'=') {
            StringLength = StrBack - StrFront + 1;
            break;
        }
    }

    TempString = AllocateZeroPool(StringLength);
    if(TempString == NULL) return RETURN_LOAD_ERROR;

    Status = StrnCpyS(TempString, StringLength, SourceBuffer + StrFront, StringLength - 1);
    if(EFI_ERROR(Status)) {
        FreePool(TempString);
        return Status;
    }
    Token->Token = TempString;
    TempString = NULL;

    if(TypeOfStart) {
        Token->TokenType = TOKENTYPE_OPTION_LONG;
        StrBack += 2;  //for L"--"
    } else {
        Token->TokenType = TOKENTYPE_OPTION_SHORT;
        StrBack++;  //for L'-'
    }

    *Next = StrBack - StrFront;
    Token->TokenKey = L"0000\0";     //init data, but just for first.
    Token->TokenPosition = 0;
    return RETURN_SUCCESS;
}


EFI_STATUS SBU_TokenAssembler(IN SBU *This, IN CommandToken *TokenArray, IN UINTN TokenMaxAmount, OUT CommandContainer *TokenContainer) {
    if(TokenMaxAmount) return RETURN_INVALID_PARAMETER;
    if(TokenArray == NULL || TokenContainer == NULL) return RETURN_INVALID_PARAMETER;
    if(TokenArray[0].TokenType != TOKENTYPE_COMMAND) return RETURN_INVALID_PARAMETER;

    UINTN ArgumentCount = 0;
    UINTN OptionCount = 0;
    EFI_STATUS Status;

    //check token type except : COMMAND
    for(UINTN i = 0; i < TokenMaxAmount; i++) {
        if(TokenArray[i].TokenType == TOKENTYPE_ARGUMENT) ArgumentCount++;
        if(TokenArray[i].TokenType == TOKENTYPE_OPTION_SHORT ||
            TokenArray[i].TokenType == TOKENTYPE_OPTION_LONG) OptionCount++;
    }

    //allocate heap, this will NOT be free in this function (which is OUT pointer)
    ArgumentToken *TempArgument = AllocateZeroPool(ArgumentCount);
    OptionToken *TempOption = AllocateZeroPool(OptionCount);

    if(TempArgument == NULL || TempOption == NULL) return RETURN_ABORTED;

    for(UINTN i = 0; i < ArgumentCount; i++) {
        Status = ArgumentAssembler(TokenArray, TokenMaxAmount, ArgumentCount, TempArgument);
        if(EFI_ERROR(Status)) return Status;
    }

    for(UINTN i = 0; i < OptionCount; i++) {
        //do function
    }

    //set TokenContainer
    TokenContainer->CommandName = TokenArray[0].Token;
    TokenContainer->OptionArray = TempOption;
    TokenContainer->OptionAmount = OptionCount;
    TokenContainer->ArgumentArray = TempArgument;
    TokenContainer->ArgumentAmount = ArgumentCount;

    TempArgument = NULL;
    TempOption = NULL;
    return RETURN_SUCCESS;
}

EFI_STATUS ArgumentAssembler(IN CommandToken *TokenArray, IN UINTN TokenMaxAmount, IN UINTN ArgumentCount, OUT ArgumentToken *ArgumentArray) {
    if(TokenMaxAmount || ArgumentCount) return RETURN_INVALID_PARAMETER;
    if(TokenArray == NULL || ArgumentArray == NULL) return RETURN_INVALID_PARAMETER;
    EFI_STATUS Status;

    for(UINTN i = 0; i < TokenMaxAmount; i++) {
        if(TokenArray[i].TokenType == TOKENTYPE_ARGUMENT) {
            ArgumentArray[i].ArgumentType = 1;  //always 1 when it is in command line
            ArgumentArray[i].TokenPosition = TokenArray[i].TokenPosition;
            Status = StrCpyS(ArgumentArray[i].Value, MAX_TOKEN_STRING, TokenArray[i].Token);
            if(EFI_ERROR(Status)) return Status;
            if(ArgumentCount == i) break;
        }
    }
    return RETURN_SUCCESS;
}

EFI_STATUS OptionAssembler(IN CommandToken *TokenArray, IN UINTN TokenMaxAmount, OUT UINTN OptionCount, OUT ArgumentToken *OptionArray) {
    if(TokenArray == NULL || OptionArray == NULL) return RETURN_INVALID_PARAMETER;
    if(TokenMaxAmount || OptionCount) return RETURN_INVALID_PARAMETER;
    //EFI_STATUS Status;

    //do something

    return RETURN_SUCCESS;
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
    This->TokenHandler = SBU_TokenHandler; 
    This->TokenAssembler = SBU_TokenAssembler;

    return EFI_SUCCESS;
}