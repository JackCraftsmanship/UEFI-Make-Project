#ifndef _ZCP_UEFI_BootFileSystemUtility_
#define _ZCP_UEFI_BootFileSystemUtility_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

/** This Object is renamed identifier of _Boot_File_System_Utility.
 * It will give the interface of File System via EFI Simple File System Protocol
 * @note when init, attach with this function : BFSU_InitializeLib
 */
typedef struct _Boot_File_System_Utility BFSU;

struct _Boot_File_System_Utility {

    /** This is show the current directory path on terminal.
     * use for printing path
     */
    CHAR16 *CurrentDirectoryPath;
    
    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * Get EFI_SIMPLE_FILE_SYSTEM_PROTOCOL GUID and current directory pointer
     * @note This function is for protocol initializing, not extern use
     * @param This self
     * @param FsProtocol EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
     * @param RootHandle directory handler
     * @param Status Status return
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*ProtocolHeader)(
        IN BFSU *This,
        OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **FsProtocol,
        OUT EFI_FILE_PROTOCOL **RootHandle,
        OUT EFI_STATUS *Status
    );

    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * check file name that actually follow the rule of FAT32
     * @note This function is for file name checker, not extern use
     * @param This self
     * @param FileName file name, not Path
     * @param FileNameLength the sizeof file name
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*FileNameCheck)(
        IN BFSU *This,
        IN CHAR16 *FileName,
        IN UINTN FileNameLength
    );

    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * Make file in current directory
     * @param This self
     * @param FileName file name, can accept Path
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*MakeFile)(
        IN BFSU *This,
        IN CHAR16 *FileName
    );

    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * Open file
     * @param This self
     * @param FilePath file name, can accept Path
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*FileOpen)(
        IN BFSU *This,
        IN CHAR16 *FilePath
    );

    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * Simply write letters in the back of file
     * @param This self
     * @param FilePath file name, can accept Path
     * @param StringToWriteBack the sectance that want to write.
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*SimpleFileWriteBack)(
        IN BFSU *This,
        IN CHAR16 *FilePath,
        IN CHAR16 *StringToWriteBack
    );

    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * Make Directory
     * @param This self
     * @param DirectoryName file name, can accept Path
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*MakeDirectory)(
        IN BFSU *This,
        IN CHAR16 *DirectoryName
    );

    /** This is Part of _Boot_File_System_Utility or BFSU, 
     * Goto the Directory that want to
     * @param This self
     * @param DirectoryPath file name, can accept Path
     * @return When Error, return EFI_ERROR, when Normal, return EFI_SUCCESS
     */
    EFI_STATUS (*GotoDirectory)(
        IN BFSU *This,
        IN CHAR16 *DirectoryPath
    );
};

///////////////////////////Inline Method//////////////////////////////

EFI_STATUS BFSU_ProtocolHeader(BFSU *This, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL **FsProtocol, EFI_FILE_PROTOCOL **RootHandle, EFI_STATUS *Status);

// fileName Handle Flags
#define FILE_CHECK_NORMAL 0x00
#define FILE_CHECK_VOID 0x01
#define FILE_CHECK_INVAILD_NAME 0x02
#define FILE_CHECK_TOO_LONG 0x04

EFI_STATUS BFSU_FileNameChecker(BFSU *This, CHAR16 *FileName, UINTN FileNameLength);


///////////////////////////file handler//////////////////////////////

EFI_STATUS BFSU_MakeFile(IN BFSU *This, IN CHAR16 *FileName);

////////////////////////directory handler///////////////////////////

EFI_STATUS BFSU_GotoDirectory(BFSU *This, CHAR16 *DirectoryPath);

EFI_STATUS BFSU_InitializeLib(BFSU *This);

#endif