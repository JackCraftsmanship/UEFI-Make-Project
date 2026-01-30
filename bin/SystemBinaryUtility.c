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

EFI_STATUS SBU_OptionHandler(IN SBU *This, IN CHAR16 *SourceString, IN OptionFlag OptionIdentifier[], IN UINTN OptionIdentifierCount,
            IN UINTN MaxTokenLength, IN UINTN ReturnArrayLength, OUT CHAR16 ReturnOptionTokenArray[ReturnArrayLength][MaxTokenLength]) {
    UINTN SStrSize = StrLen(SourceString);
    UINTN ROTA_index = 0;
    CHAR16 FlagFirstID[OptionIdentifierCount];
    EFI_STATUS Status;

    for(INTN i = 0; i < OptionIdentifierCount; i++) {
        if(OptionIdentifier[i].OptionIdentifier[0] != FlagFirstID[ROTA_index]) {
            FlagFirstID[ROTA_index++] = OptionIdentifier[i].OptionIdentifier[0];
        }
    }

    ROTA_index = 0;

    for(INTN i = 0; i <= SStrSize; i++) {
        for(INTN j = 0; j < StrLen(FlagFirstID); j++) {
            if(FlagFirstID[j] == SourceString[i]) goto DO_EXACT_CMP;
        }
        continue;

        DO_EXACT_CMP:
        for (INTN j = 0; j < OptionIdentifierCount; j++) {
            if(!StrnCmp(SourceString + i, OptionIdentifier[j].OptionIdentifier, OptionIdentifier[j].OptionTokenLength)) {
                if(ROTA_index >= ReturnArrayLength) return EFI_SUCCESS;
                Status = StrCpyS(ReturnOptionTokenArray[ROTA_index++], MaxTokenLength, OptionIdentifier[j].OptionIdentifier);
                Print(L"Status : %d\r\n", Status);
                if(EFI_ERROR(Status)) return Status;
                i += OptionIdentifier[j].OptionTokenLength - 1;
            }
        }
    }

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