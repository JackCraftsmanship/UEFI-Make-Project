#ifndef _ZCP_UEFI_SystemBinaryUtility_
#define _ZCP_UEFI_SystemBinaryUtility_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

/** This Object is renamed identifier of _System_Binary_Utility.
 * It will give the interface of Basic Termianl Control in Boot Service
 * @note when init, attach with this function : SBU_InitializeLib
 */
typedef struct _System_Binary_Utility SBU;

struct _System_Binary_Utility {

    /** This is Part of _System_Binary_Utility or SBU, 
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

    /** This is Part of _System_Binary_Utility or SBU, 
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

    /** This is Part of _System_Binary_Utility or SBU, 
     * Call shutdown Command.
     * @note This will be add some Options
     * @param This self
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*ShutdownCommand)(
        IN SBU *This
    );

    /** This is Part of _System_Binary_Utility or SBU, 
     * Internal Option Handler for handle Option in command.
     * @note Currently not vaild
     * @param This self
     * @param SourceString Source string that want to parsing option
     * @param OptionIdentifier Option Identifier
     * @param ReturnOptionTokenArray Return ALL Option Token that Identified, Return CHAR16 string ARRAYs
     * @param MaxTokenLength The Maximum Length of Token
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*OptionHandler)(
        IN SBU *This,
        IN CHAR16 *SourceString,
        IN CHAR16 *OptionIdentifier,
        CHAR16 *ReturnOptionTokenArray,
        IN UINTN MaxTokenLength
    );

    /** This is Part of _System_Binary_Utility or SBU, 
     * Who Am I String Printer. Suitable for testing BootService.
     * @param This self
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*WhoamI)(
        IN SBU *This
    );
};

EFI_STATUS SBU_Readline(IN SBU *This, OUT CHAR16 *Buffer, IN UINTN BufferSize);

EFI_STATUS SBU_ReBoot(IN SBU *This, IN CHAR16 *Option);

EFI_STATUS SBU_Shutdown(IN SBU *This);

EFI_STATUS SBU_OptionHandler(IN SBU *This, IN CHAR16 *SourceString, IN CHAR16 *OptionIdentifier,
                                CHAR16 *ReturnOptionTokenArray, IN UINTN MaxTokenLength);

EFI_STATUS SBU_WhoamI(IN SBU *This);

EFI_STATUS SBU_InitializeLib(IN SBU *This);

/** This is Option Flag that can be use as option identifier. 
 * @note All of option MUST HAVE Option Identifier Symbol, if not, it will be ignored.
 * @note Use this : !, @, #, $, -, ,
 */
typedef struct {
    CHAR16 *OptionIdentifier;   //base Option Identifier
    CHAR16 *OptionToken;        //actual Option Token
} OptionFlag;

typedef struct {
    OptionFlag *OptionArray;        //OptionContainer Array
    UINTN MaxInputOptionLength;          //Maximum Accept Input Option Length
    UINTN MaxInputArrayLength;           //Maximim Accept Input Option Container Array Length
 } OptionContainer;

#endif