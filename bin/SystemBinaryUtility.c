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

EFI_STATUS SBU_TokenHandler(IN SBU *This, IN CHAR16 *SourceBuffer, IN UINTN TokenMaxAmount, OUT CommandToken *Token) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;

    UINTN StrFront = 0;
    UINTN CharLength = 0;
    UINTN index = 1;
    EFI_STATUS Status;

    for(; StrFront < (StrLen(SourceBuffer) + 1); StrFront++) {
        if(SourceBuffer[StrFront] == L'\0') return EFI_SUCCESS;
        if(SourceBuffer[StrFront] == L' ') {
            if (SourceBuffer[StrFront + 1] == L'\0') return EFI_SUCCESS;
            else if(SourceBuffer[StrFront + 1] == L'-') {
                StrFront++;
                Status = Token_OptionHandler(SourceBuffer + StrFront, &Token[index], &CharLength);
                StrFront += CharLength;
                Token[index].TokenPosition = index;
                index++;
                if(EFI_ERROR(Status)) return Status;
            }
            else if(SourceBuffer[StrFront + 1] != L' ') {
                Status = Token_ArgumentHandler(SourceBuffer + StrFront, &Token[index], &CharLength);
                StrFront += CharLength;
                Token[index].TokenPosition = index;
                index++;
                if(EFI_ERROR(Status)) return Status;
            }
        }
        if(index >= TokenMaxAmount) return EFI_SUCCESS;
    }
    return EFI_SUCCESS;
}

EFI_STATUS Token_ArgumentHandler(IN CHAR16 *SourceBuffer, OUT CommandToken *Token, OUT UINTN *Next) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;

    UINTN StrBack = 0;
    UINTN StrFront = 0;
    EFI_STATUS Status;

    while(TRUE) {
        if(SourceBuffer[StrBack] == L' ') {
            if(StrBack == StrFront) {
                StrBack++;
                StrFront++;
                continue;
            }
            Status = StrnCpyS(Token->Token, MAX_TOKEN_STRING, SourceBuffer + StrFront, StrBack - StrFront);
            if(EFI_ERROR(Status)) return Status;
            break;
        }
        else if(SourceBuffer[StrBack] == L'\0') {
            Status = StrnCpyS(Token->Token, MAX_TOKEN_STRING, SourceBuffer + StrFront, StrBack - StrFront);
            if(EFI_ERROR(Status)) return Status;
            break;
        }
        StrBack++;
        if(StrBack >= MAX_TOKEN_STRING) return RETURN_NOT_FOUND;
        if(SourceBuffer[StrBack] == L'\0' && StrBack == StrFront) return RETURN_NOT_FOUND;
    }
    Token->TokenType = TOKENTYPE_ARGUMENT;
    Token->TokenPosition = 0;

    Print(L"Return Pointer vaule : %d\r\n", StrBack - StrFront);
    *Next = StrBack - StrFront;
    return EFI_SUCCESS;
}

EFI_STATUS Token_OptionHandler(IN CHAR16 *SourceBuffer, OUT CommandToken *Token, OUT UINTN *Next) {
    if(StrSize(SourceBuffer) == 0) return RETURN_BAD_BUFFER_SIZE;
    if(SourceBuffer[0] == L'\0') return RETURN_BAD_BUFFER_SIZE;

    UINTN StrBack = 0;
    UINTN StrFront = 0;
    EFI_STATUS Status;

    /*
    for(; StrFront < StrLen(SourceBuffer) + 1; StrFront++) {
        if(SourceBuffer[StrFront] == L'-') break;
    }
    */

    if(!StrnCmp(SourceBuffer + StrFront, L"--", 2)) {
        StrFront += 2;
        StrBack = StrFront;
        if(SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L' ') return RETURN_INVALID_PARAMETER;

        StrBack++;
        while(TRUE) {
        if(SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L' ') {
            Status = StrnCpyS(Token->Token, MAX_TOKEN_STRING, SourceBuffer + StrFront, StrBack - StrFront);
            if(EFI_ERROR(Status)) return Status;
            break;
        }
        StrBack++;
        if(StrBack >= MAX_TOKEN_STRING) return RETURN_INVALID_PARAMETER;
        if(SourceBuffer[StrBack] == L'\0' && StrBack == StrFront) return RETURN_INVALID_PARAMETER;
        }
        Token->TokenType = TOKENTYPE_OPTION_LONG;
    }
    
    else {
        StrFront += 1;
        StrBack = StrFront;
        if(SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L' ') return RETURN_INVALID_PARAMETER;

        StrBack++;
        while(TRUE) {
        if(SourceBuffer[StrBack] == L'\0' || SourceBuffer[StrBack] == L' ') {
            Status = StrnCpyS(Token->Token, MAX_TOKEN_STRING, SourceBuffer + StrFront, StrBack - StrFront);
            if(EFI_ERROR(Status)) return Status;
            break;
        }
        StrBack++;
        if(StrBack >= MAX_TOKEN_STRING) return RETURN_INVALID_PARAMETER;
        if(SourceBuffer[StrBack] == L'\0' && StrBack == StrFront) return RETURN_INVALID_PARAMETER;
        }
        Token->TokenType = TOKENTYPE_OPTION_SHORT;
    }
    Print(L"Return Pointer vaule : %d\r\n", StrBack - StrFront);
    *Next = StrBack - StrFront;
    Token->TokenPosition = 0;
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
    This->TokenHandler = SBU_TokenHandler; 

    return EFI_SUCCESS;
}