#include "./StringHandle.h"

EFI_STATUS SubStr(IN CHAR16 *SourceString, OUT CHAR16 *DestinationString, IN UINTN Start) {
    UINTN F_size = StrSize(SourceString);

    if(SourceString[0] == L'\0') return EFI_INVALID_PARAMETER;

    else if(Start >= (F_size)) return EFI_BAD_BUFFER_SIZE;

    for(INTN i = Start; i <= F_size; i++) {
        DestinationString[i - Start] = SourceString[i];
    }
    return EFI_SUCCESS;
}

EFI_STATUS SubnStr(IN CHAR16 *SourceString, OUT CHAR16 *DestinationString, IN UINTN Start, IN UINTN End) {
    UINTN F_size = StrSize(SourceString);
    UINTN D_size = StrSize(DestinationString);

    if(SourceString[0] == L'\0') return EFI_INVALID_PARAMETER;

    if(End < Start) return EFI_INVALID_PARAMETER;
    else if(Start >= (F_size)) return EFI_BAD_BUFFER_SIZE;
    else if(End >= (F_size)) return EFI_BAD_BUFFER_SIZE; 
    if((End - Start + 1) > D_size) return EFI_BUFFER_TOO_SMALL;

    for(INTN i = Start; i <= End; i++) {
        DestinationString[i - Start] = SourceString[i];
    }
    return EFI_SUCCESS;
}