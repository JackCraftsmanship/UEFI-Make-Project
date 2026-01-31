#ifndef _ZCP_UEFI_SystemBinaryUtility_
#define _ZCP_UEFI_SystemBinaryUtility_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define ShortOption "-"
#define LongOption "--"
#define MAX_TOKEN_STRING 256

/*
구조를 이런식으로 만들기 : 
명령어 [토큰1] [토큰2]

토큰1이 인자이면 -> 인자 해석기로 보내기
토큰1이 옵션이면 -> 옵션 해석기로 보내기

마찬가지로 각 토큰마다 하기, 그리고 토큰는 L" "에 의해 나누어짐

각 토큰 번호를 내보내는 인자/옵션 포인터 배열에 매기기
토큰는 각 토큰들의 위치 관계를 정확하게 나타내줄 것임
*/

/** This is Document type for Argument, contain Key : value
 * each argument is separated by " " and NOT start with "-" or "--"
 */
typedef struct {
    UINTN ArgumentType;     //ArgumentType determine which argument is nessesary or not, 1 for Need, 0 for Optional
    CHAR16 *Key;            //Key of Argument, Key will hold argument identifier
    CHAR16 Value[MAX_TOKEN_STRING];          //Value of Argument, Value will hold argument value
} ArgumentFlag;

/** This is Option Flag that can be use as option identifier. Contain ONE option flag data structure
 * @note All of option MUST HAVE Option Identifier Symbol, if not, it is difficult to identify it
 * @note start with this : "-" or "--"
 */
typedef struct {
    BOOLEAN OptionType;             //Option Type of Token, use 0 for short, 1 for long
    CHAR16 *OptionToken;            //Option Token
} OptionFlag;

#define TOKENTYPE_COMMAND 0xf1
#define TOKENTYPE_ARGUMENT 0xF2
#define TOKENTYPE_OPTION_SHORT 0xF3
#define TOKENTYPE_OPTION_LONG 0xF4

/** This Token of Command, Which is the output of Token Interpreter.
 * It hold ONE token, and it's position in command line.
 * It will send to Command program for further use.
 */
typedef struct {
    CHAR16 Token[MAX_TOKEN_STRING];
    UINTN TokenType;                    //can accept : TOKENTYPE_s
    UINTN TokenPosition;
} CommandToken;

/** This is Command Container that can be Contain some Option Identifiers and set some rules about Options
 * @note Argument Array index will be the position of Argument when input
 */
typedef struct {
    CHAR16 *CommandName;            //Main Command name
    OptionFlag *OptionArray;        //Option Array
    UINTN OptionAmount;             //The amount of Option that option will handle
    ArgumentFlag *ArgumentArray;    //Argument array that Command can handle
    UINTN ArgumentAmount;           //The amount of Argument that option will handle, if it held New Command, set to 1
 } CommandContainer;

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
     * @param SourceBuffer Command line Buffer
     * @param TokenMaxAmount Token amount, it determined by the (OptionAmount + ArgumentAmount + 1) in CommandContainer
     * @param Token Actual Token Array, MUST same amount as TokenMaxAmount does
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*TokenHandler)(
        IN SBU *This,
        IN CHAR16 *SourceBuffer,
        IN UINTN TokenMaxAmount,
        OUT CommandToken *Token
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

#define OPTION_MAX_LENGTH 128

EFI_STATUS SBU_TokenHandler(IN SBU *This, IN CHAR16 *SourceBuffer, IN UINTN TokenMaxAmount, OUT CommandToken *Token);
EFI_STATUS Token_ArgumentHandler(IN CHAR16 *SourceBuffer, OUT CommandToken *Token, OUT UINTN *Next);
EFI_STATUS Token_OptionHandler(IN CHAR16 *SourceBuffer, OUT CommandToken *Token, OUT UINTN *Next);

EFI_STATUS SBU_WhoamI(IN SBU *This);

EFI_STATUS SBU_InitializeLib(IN SBU *This);

#endif