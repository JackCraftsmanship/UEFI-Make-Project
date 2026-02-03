#ifndef _ZCP_UEFI_SystemBinaryUtility_
#define _ZCP_UEFI_SystemBinaryUtility_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>

#define ShortOption "-"
#define LongOption "--"
#define MAX_TOKEN_STRING 256

//나중에 구조체의 유효성 검사(댕글링 포인터 방지)를 위한
//#define OPTION_TOKEN_SIGNATURE  SIGNATURE_32 ('O', 'P', 'T', 'K') 
// 사용한 시그니쳐 판독법을 쓸 것. 이걸로 재대로된 인수가 들어오는지도 확인 가능 (배열의 갯수)

/*
토큰 핸들러 구조를 이런식으로 만들기 : 
명령어 [토큰1] [토큰2]

토큰1이 인자이면 -> 인자 해석기로 보내기
토큰1이 옵션이면 -> 옵션 해석기로 보내기

마찬가지로 각 토큰마다 하기, 그리고 토큰은 L" "에 의해 나누어짐 (아닐 경우, 파싱 실패, 명령어 동작 안함.)

각 토큰 번호를 내보내는 인자/옵션 포인터 배열에 매기기
토큰는 각 토큰들의 위치 관계를 정확하게 나타내줄 것임

인자 토큰의 경우, L'\"'로 띄어쓰기 포함 가능
*/

/** This Token of Command, Which is the output of Token Interpreter.
 * It hold ONE token, and it's position in command line.
 * It will send to Command program for further use.
 */
#define C_Token_Signature SIGNATURE_32('T', 'K', 'C', 'L')

typedef struct C_Token{
    UINT32 Signature;                   //Token Signature Verifier
    LIST_ENTRY Link;                    //List Link data for EDK2 DL-R-List
    UINTN TokenType;                    //can accept : TOKENTYPE_*
    CHAR16 *TokenKey;                   //Token identifier for Argument identifier, when first == L'\0', ignored
    CHAR16 *Token;                      //actual token
    UINTN TokenPosition;                //position of token
} CommandToken;

#define TOKENTYPE_COMMAND 0xf1
#define TOKENTYPE_ARGUMENT 0xF2
#define TOKENTYPE_OPTION_SHORT 0xF3
#define TOKENTYPE_OPTION_LONG 0xF4

/*
토큰 작성기를 이런식으로 만들기 : 

들어온 각 토큰들의 타입을 검사 : TOKENTYPE_* 를 사용하면 됨.
구조에 맞추어 미리 만들어진 양식, CommandContainer 에 따라 작성.

들어온 인자 중 하나라도 지정된 양식에 맞지 않다면, 에러를 내보냄.

명령어는 정해진 인자 갯수가 있으며, 총 3가지임.
인자 - 명령어의 인자.
서브 명령어 - 명령어의 인자이나, 이들의 세부 동작을 규정하는 것. 있다면 필수 사항으로 기재
옵션 - 명령어의 동작 방식 결정, 옵션은 추가 인자를 요구할 수 있음.

명령어는 단일이 아닌 이상, 인자나 옵션을 적어도 하나 이상을 받아야 하며, 무조건 옵션이 먼저 오도록 해야됨.

명령어 작성 규칙 : 명령어 {서브 명령어} {옵션}={옵션 인자} {서브 커멘드 또는 명령어 인자}
서브 명령어에 띄어쓰기는 포함할 수 없다.
인자나 옵션 인자는 L'\"'를 통해 띄어쓰기를 포함한 문자들의 나열을 받을 수 있다.
옵션이 인자를 받는 방법 : 옵션은 통상적으로 L'='를 통해 옵션 인자를 받는다.
만약, 옵션이 여러 인자를 원하는 경우, 인자는 띄어쓰기를 통해 받으며, 다음에 오는 일반 인자는 옵션에 종속된다.


옵션에서, 값이 없는 경우에는 bool 타입으로 간주, 스위칭 기능을 수행.
값이 있으면 추가적으로 값이 들어오는데, 이 값을 검사하지는 않음. 이때는 문자열 취급.

작성기는 종류 판단과 형식 작성을 하는 것일 뿐, 실제 해석은 각 명령어에서 제공한 JSON을 토대로 진행할 것.
*/

/** This is Document type for Argument, contain Key : value
 * each argument is separated by " " and NOT start with "-" or "--"
 */
typedef struct C_Argument{
    UINTN TokenPosition;                //Position in the Command line String
    CHAR16 *Key;                        //Key value that identify the data, when first CHAR16 is L'\0', ignored
    UINTN ArgumentType;                 //ArgumentType determine which argument is nessesary or not, 1 for Need, 0 for Optional, if above, ignore
    CHAR16 Value[MAX_TOKEN_STRING];     //Value of Argument, Value will hold argument value, if 0, ignored
} ArgumentToken;

/** This is Option Flag that can be use as option identifier. Contain ONE option flag data structure
 * @note All of option MUST HAVE Option Identifier Symbol, if not, it is difficult to identify it
 * @note start with this : "-" or "--"
 */
typedef struct C_Option{
    UINTN TokenPosition;            //Position in the Command line String, if 0, ignored
    BOOLEAN OptionType;             //Option Type of Token, use 0 for short, 1 for long
    CHAR16 *OptionToken;            //Option Token
    UINTN OptionValueType;          //The Type of Option Value, 0 for OPBool, 1 for OPArgument, 2 for OPArgumentAmount
    union OptionData {              //Option Value, {union} : type that can be selected
        BOOLEAN OPBool;             //OPBool is the bool type data which work as switch
        CHAR16 OPArgument[MAX_TOKEN_STRING];     //OPArgument is the CHAR16 type String which store the argument follow with L'='
        UINTN OPArgumentAmount;     //OPArgumentAmount is the CHAR16 type UINTN which store the follow argument amount that want to handle
    } OptionValue;      //because of C99, MUST write union Name for proper use
} OptionToken;

/** This is Command Container that can be Contain some Option Identifiers and set some rules about Options.
 * @note All the position of Token is held by self; which can observed by TokenPosition member.
 */
typedef struct C_Container{
    CHAR16 *CommandName;            //Main Command name
    OptionToken *OptionArray;       //Option Array
    UINTN OptionAmount;             //The amount of Option that option will handle
    ArgumentToken *ArgumentArray;   //Argument array that Command can handle
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
     * @param SourceBuffer The Source string, it is not parsing the CommandToken
     * @param TokenMaxAmount Max amount tokens that Token Array can handle, 0 for infinite amount
     * @param TokenArrayPointer Token Array Pointer
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*TokenHandler)(
        IN SBU *This,
        IN CHAR16 *SourceBuffer,
        IN UINTN TokenMaxAmount,
        IN OUT LIST_ENTRY *TokenArrayPointer
    );

    /** This is Part of _System_Binary_Utility or SBU, 
     * Internal Option Assembler for handle Option in command.
     * @note Currently not vaild
     * @param This self
     * @param TokenArray The Source TokenArray, it is not parsing the CommandToken
     * @param TokenMaxAmount Max amount tokens that Token Array can handle
     * @param TokenContainer Assembled Token Container that ready to excute command
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*TokenAssembler)(
        IN SBU *This,
        IN CommandToken *TokenArray,
        IN UINTN TokenMaxAmount,
        OUT CommandContainer *TokenContainer
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

EFI_STATUS SBU_TokenHandler(IN SBU *This, IN CHAR16 *SourceBuffer, IN UINTN TokenMaxAmount, IN OUT LIST_ENTRY *TokenArrayPointer);
EFI_STATUS Token_ArgumentHandler(IN CHAR16 *SourceBuffer, IN OUT CommandToken *Token, OUT UINTN *Next);
EFI_STATUS Token_OptionHandler(IN CHAR16 *SourceBuffer, IN OUT CommandToken *Token, OUT UINTN *Next);

VOID Token_List_Destructor(IN LIST_ENTRY *ListEntryPointer);

EFI_STATUS SBU_TokenAssembler(IN SBU *This, IN CommandToken *TokenArray, IN UINTN TokenMaxAmount, OUT CommandContainer *TokenContainer);
EFI_STATUS ArgumentAssembler(IN CommandToken *TokenArray, IN UINTN TokenMaxAmount, IN UINTN ArgumentCount, OUT ArgumentToken *ArgumentArray);
EFI_STATUS OptionAssembler(IN CommandToken *TokenArray, IN UINTN TokenMaxAmount, IN UINTN OptionCount, OUT ArgumentToken *OptionArray);

EFI_STATUS SBU_WhoamI(IN SBU *This);

EFI_STATUS SBU_InitializeLib(IN SBU *This);

#endif