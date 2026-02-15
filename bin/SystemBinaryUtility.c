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

    if(!StrCmp(Option, L"c") || !StrCmp(Option, L"cold")) {
        Print(L"Reset with device shutdown.");
        resetType = EfiResetCold;
    }
    else if(!StrCmp(Option, L"w") || !StrCmp(Option, L"warm")) {
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

EFI_STATUS SBU_TokenHandler(IN SBU *This, IN CHAR16 *SourceBuffer, IN UINTN TokenMaxAmount, IN OUT LIST_ENTRY **TokenArrayPointer) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;
    if (TokenArrayPointer == NULL) return RETURN_INVALID_PARAMETER;

    UINTN StrFront = 0;
    UINTN CharLength = 0;
    UINTN index = 1;
    UINTN StrLength = StrLen(SourceBuffer) + 1;
    UINT8 TokenTypeFlag = 0;
    BOOLEAN StartParsingFlag = FALSE;       //toggler

    LIST_ENTRY *ListInitHead = AllocateZeroPool(sizeof(LIST_ENTRY));
    if(ListInitHead == NULL) {
        DEBUG((DEBUG_ERROR, "(Error) : Cannot Allocate Memory\r\n"));
        return RETURN_OUT_OF_RESOURCES;
    }
    InitializeListHead(ListInitHead);

    CommandToken *TokenNode;
    EFI_STATUS Status;

    for(; StrFront < StrLength; StrFront++) {
        //CK string
        if(SourceBuffer[StrFront] == L'\0') break;
        if(SourceBuffer[StrFront] == L' ') {
            if (SourceBuffer[StrFront + 1] == L'\0') break;
            if (SourceBuffer[StrFront + 1] == L'=') goto SKIP_FOR_1;
            StrFront++;     //skip L' '
            if(SourceBuffer[StrFront] == L'-') {
                if(SourceBuffer[StrFront + 1] == L'-') {
                    TokenTypeFlag = TOKENTYPE_OPTION_LONG;     //identify option_long
                    StartParsingFlag = TRUE;
                } else {
                    TokenTypeFlag = TOKENTYPE_OPTION_SHORT;     //identify option_short
                    StartParsingFlag = TRUE;
                }
            }
            
            else if(SourceBuffer[StrFront] != L' ') {
                TokenTypeFlag = TOKENTYPE_ARGUMENT;     //identify argument
                StartParsingFlag = TRUE;
            }
        }
        
        else if(SourceBuffer[StrFront] == L'=') {
            SKIP_FOR_1:
            if(index <= 1) return RETURN_INVALID_PARAMETER;
            TokenTypeFlag = TOKENTYPE_ADDITION_ARGUMENT;     //identify Additional argument
            StartParsingFlag = TRUE;
        }
        else if(TokenMaxAmount != 0 && index >= TokenMaxAmount) break;
        else return RETURN_INVALID_PARAMETER;

        //start parsing token when enable : StartParsingFlag
        if(StartParsingFlag) {
            //add TokenNode Pool to heap
            TokenNode = AllocateZeroPool(sizeof(CommandToken));
            if(TokenNode == NULL) return RETURN_LOAD_ERROR;

            DEBUG((DEBUG_INFO, "Token Parsing start : index = %d\r\n", index));
            if(TokenTypeFlag == TOKENTYPE_OPTION_SHORT) {
                Status = Token_OptionHandler(SourceBuffer + StrFront, TokenNode, &CharLength, TokenTypeFlag);
                if(EFI_ERROR(Status)) goto TOKENHANDLER_FAILSAFE;
                if(CharLength < 2) goto TOKENHANDLER_FAILSAFE;

                StrFront += CharLength;
                DEBUG((DEBUG_INFO, "Parsing End with Length : %d\r\n Next Char : \'%c\'\r\n", CharLength, SourceBuffer[StrFront]));
                StrFront--;

                for(UINTN i = 0; i < CharLength - 1; i++) {
                    CommandToken *TempToken = AllocateZeroPool(sizeof(CommandToken));
                    if(TempToken == NULL) return RETURN_OUT_OF_RESOURCES;

                    TempToken->Signature = C_Token_Signature;
                    TempToken->TokenKey = AllocateZeroPool(sizeof(CHAR16));
                    if(TempToken->TokenKey == NULL) return RETURN_OUT_OF_RESOURCES;
                    TempToken->TokenKey[0] = L'\0';         //cause handler doesn't use this
                    TempToken->TokenPosition = index;
                    TempToken->Token = AllocateZeroPool(sizeof(CHAR16) * 2);
                    if(TempToken->Token == NULL) return RETURN_OUT_OF_RESOURCES;
                    TempToken->Token[0] = TokenNode->Token[i];
                    TempToken->Token[1] = L'\0';
                    TempToken->TokenType = TOKENTYPE_OPTION_SHORT;

                    DEBUG((DEBUG_INFO, "Token Separated : %d, %s\r\n", TempToken->TokenPosition, TempToken->Token));

                    InsertTailList(ListInitHead, &TempToken->Link);
                    index++;
                }
                StartParsingFlag = FALSE;
                FreePool(TokenNode);    //only free this, not inside : Token and TokenKey are still in the pool(part of TempToken->TokenKey)
                continue;
            }
            else if(TokenTypeFlag == TOKENTYPE_OPTION_LONG) Status = Token_OptionHandler(SourceBuffer + StrFront, TokenNode, &CharLength, TokenTypeFlag);
            else Status = Token_ArgumentHandler(SourceBuffer + StrFront, TokenNode, &CharLength, TokenTypeFlag);
            if(EFI_ERROR(Status)) goto TOKENHANDLER_FAILSAFE;

            StrFront += CharLength;
            DEBUG((DEBUG_INFO, "Parsing End with : %d\r\n Next Char : \'%c\'\r\n", StrFront, SourceBuffer[StrFront]));
            StrFront--;
            TokenNode->TokenPosition = index;
            TokenNode->Signature = C_Token_Signature;

            InsertTailList(ListInitHead, &TokenNode->Link);

            index++;
            StartParsingFlag = FALSE;
        }
    }

    *TokenArrayPointer = ListInitHead;
    ListInitHead = NULL;
    return RETURN_SUCCESS;

TOKENHANDLER_FAILSAFE:
    DEBUG((DEBUG_INFO, "Error : TokenParsing Failed"));
    Token_List_Destructor(ListInitHead);
    if(TokenNode != NULL) {
        if (TokenNode->Token != NULL) {
            FreePool (TokenNode->Token);
        }

        if (TokenNode->TokenKey != NULL) {
            FreePool (TokenNode->TokenKey);
        }
        FreePool(TokenNode);
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

        DEBUG((DEBUG_INFO, "Delete Token : %s\r\n", Entry->Token));
        if (Entry->Token != NULL) {
        FreePool (Entry->Token);
        }

        DEBUG((DEBUG_INFO, "Delete TokenKey : %s\r\n", Entry->TokenKey));
        if (Entry->TokenKey != NULL) {
        FreePool (Entry->TokenKey);
        }

        RemoveEntryList (CurrentLink);
        DEBUG((DEBUG_INFO, "Delete TokenNode\r\n"));
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

    TempString = AllocateZeroPool(StringLength * sizeof(CHAR16));
    if(TempString == NULL) return RETURN_OUT_OF_RESOURCES;

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

    Token->TokenKey = AllocateZeroPool(sizeof(CHAR16));     //init data, but just for first.
    if(Token->TokenKey == NULL) return RETURN_OUT_OF_RESOURCES;
    Token->TokenKey[0] = L'\0';                             //cause it just have one space for CHAR16
    Token->TokenType = ArgumentType;
    Token->TokenPosition = 0;

    *Next = StrBack - StrFront;
    return RETURN_SUCCESS;
}

EFI_STATUS Token_OptionHandler(IN CHAR16 *SourceBuffer, IN OUT CommandToken *Token, OUT UINTN *Next, IN UINTN ArgumentType) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;

    UINTN TypeOfStart = FALSE;      //L"--" == TRUE, L'-' == FALSE
    UINTN StrBack = 0;
    UINTN StrFront = 0;
    UINTN StrLength = StrLen(SourceBuffer) + 1;
    CHAR16 *TempString;
    UINTN StringLength;
    EFI_STATUS Status;

    if(ArgumentType == TOKENTYPE_OPTION_LONG) {
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

    TempString = AllocateZeroPool(StringLength * sizeof(CHAR16));
    if(TempString == NULL) return RETURN_OUT_OF_RESOURCES;

    Status = StrnCpyS(TempString, StringLength, SourceBuffer + StrFront, StringLength - 1);
    if(EFI_ERROR(Status)) {
        FreePool(TempString);
        return Status;
    }
    Token->Token = TempString;
    TempString = NULL;

    if(TypeOfStart) StrBack += 2;  //for L"--"
    else  StrBack++;  //for L'-'
    *Next = StrBack - StrFront;

    Token->TokenType = ArgumentType;
    Token->TokenKey = AllocateZeroPool(sizeof(CHAR16));     //init data, but just for first.
    if(Token->TokenKey == NULL) return RETURN_OUT_OF_RESOURCES;
    Token->TokenKey[0] = L'\0';                             //cause it just have one space for CHAR16
    Token->TokenPosition = 0;
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

    return EFI_SUCCESS;
}