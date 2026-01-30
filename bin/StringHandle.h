#ifndef _ZCP_UEFI_STRINGHANDLE_
#define _ZCP_UEFI_STRINGHANDLE_

#include <Uefi.h>
#include <Library/UefiLib.h>

/** SubString, Copy substring Start to End-of-String and paste into Destiantion
 * @param SourceString String of Origin source
 * @param DestinationString String for paste
 * @param Start starting position (contain)
 * @return EFI_ERROR code, when Sucess, return EFI_SUCCESS
*/
EFI_STATUS SubStr(IN CHAR16 *SourceString, OUT CHAR16 *DestinationString, IN UINTN Start);

/** SubString, Copy substring Start to End and paste into Destiantion
 * @param SourceString String of Origin source
 * @param DestinationString String for paste
 * @param Start starting position (contain)
 * @param End end position (contain)
 * @return EFI_ERROR code, when Sucess, return EFI_SUCCESS
*/
EFI_STATUS SubnStr(IN CHAR16 *SourceString, OUT CHAR16 *DestinationString, IN UINTN Start, IN UINTN End);

#endif