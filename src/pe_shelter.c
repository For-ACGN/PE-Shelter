#include "c_types.h"
#include "windows_t.h"
#include "hash_api.h"
#include "lib_memory.h"
#include "pe_shelter.h"

typedef struct {
    // Arguments
    uintptr ImageAddr;

    PEShelter_Opts* Options;

    // API addresses
    VirtualAlloc_t          VirtualAlloc;
    VirtualFree_t           VirtualFree;
    VirtualProtect_t        VirtualProtect;
    LoadLibraryA_t          LoadLibraryA;
    FreeLibrary_t           FreeLibrary;
    GetProcAddress_t        GetProcAddress;
    FlushInstructionCache_t FlushInstructionCache;
    CreateThread_t          CreateThread;

    // PE image information
    uintptr PEImage;
    uint32  PEOffset;
    uint16  NumSections;
    uint16  OptHeaderSize;
    uintptr DataDir;
    uintptr EntryPoint;
    uintptr ImageBase;
    uint32  ImageSize;
    uintptr ImportTable;

    uint ExitCode;

    uintptr Debug;
} PEShelterRT;

static bool initAPI(PEShelterRT* runtime);
static bool parsePEImage(PEShelterRT* runtime);
static bool mapPESections(PEShelterRT* runtime);
static bool fixRelocTable(PEShelterRT* runtime);
static bool processIAT(PEShelterRT* runtime);
static bool callEntryPoint(PEShelterRT* runtime);

uintptr LoadPE(uintptr address, PEShelter_Opts* opts)
{
    PEShelterRT runtime = {
        .ImageAddr = address,
        .Options   = opts,

        // ignore Visual Studio bug fix
        .VirtualAlloc   = (VirtualAlloc_t)1,
        .VirtualFree    = (VirtualFree_t)1,
        .VirtualProtect = (VirtualProtect_t)1,
        .LoadLibraryA   = (LoadLibraryA_t)1,
        .FreeLibrary    = (FreeLibrary_t)1,
        .GetProcAddress = (GetProcAddress_t)1,
        .FlushInstructionCache = (FlushInstructionCache_t)1,
        .CreateThread   = (CreateThread_t)1,
    };
    if (!initAPI(&runtime))
    {
        return NULL;
    }
    runtime.GetProcAddress = opts->GetProcAddress;

    for (;;)
    {
        if (!parsePEImage(&runtime))
        {
            break;
        }
        if (!mapPESections(&runtime))
        {
            break;
        }
        if (!fixRelocTable(&runtime))
        {
            break;
        }
        if (!processIAT(&runtime))
        {
            break;
        }
        runtime.Debug = (uintptr)(&callEntryPoint);
        callEntryPoint(&runtime);

        break;
    }
    // if (runtime.PEImage != NULL)
    // {
    // 
    // }
    // else
    // {
    // 
    // }
    // return runtime.ExitCode;
    return runtime.Debug;
}

// initAPI is used to find API addresses for PE loader.
static bool initAPI(PEShelterRT* runtime)
{
    #ifdef _WIN64
    uint64 hash = 0xB6A1D0D4A275D4B6;
    uint64 key  = 0x64CB4D66EC0BEFD9;
    #elif _WIN32
    uint32 hash = 0xC3DE112E;
    uint32 key  = 0x8D9EA74F;
    #endif
    VirtualAlloc_t virtualAlloc = (VirtualAlloc_t)FindAPI(hash, key);
    if (virtualAlloc == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0xB82F958E3932DE49;
    key  = 0x1CA95AA0C4E69F35;
    #elif _WIN32
    hash = 0xFE192059;
    key  = 0x397FD02C;
    #endif
    VirtualFree_t virtualFree = (VirtualFree_t)FindAPI(hash, key);
    if (virtualFree == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0x8CDC3CBC1ABF3F5F;
    key  = 0xC3AEEDC9843D7B34;
    #elif _WIN32
    hash = 0xD41DCE2B;
    key  = 0xEB37C512;
    #endif
    VirtualProtect_t virtualProtect = (VirtualProtect_t)FindAPI(hash, key);
    if (virtualProtect == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0xC0B89BE712EE4C18;
    key  = 0xF80CA8B02538CAC4;
    #elif _WIN32
    hash = 0xCCB2E46E;
    key  = 0xAEB5A665;
    #endif
    LoadLibraryA_t loadLibraryA = (LoadLibraryA_t)FindAPI(hash, key);
    if (loadLibraryA == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0xC22B47E9D652D287;
    key  = 0xA118770E82EB0797;
    #elif _WIN32
    hash = 0x2C1F810D;
    key  = 0xCB356B02;
    #endif
    FreeLibrary_t freeLibrary = (FreeLibrary_t)FindAPI(hash, key);
    if (freeLibrary == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0xB1AE911EA1306CE1;
    key  = 0x39A9670E629C64EA;
    #elif _WIN32
    hash = 0x5B4DC502;
    key  = 0xC2CC2C19;
    #endif
    GetProcAddress_t getProcAddress = (GetProcAddress_t)FindAPI(hash, key);
    if (getProcAddress == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0x8172B49F66E495BA;
    key  = 0x8F0D0796223B56C2;
    #elif _WIN32
    hash = 0x87A2CEE8;
    key  = 0x42A3C1AF;
    #endif
    FlushInstructionCache_t flushInstCache = (FlushInstructionCache_t)FindAPI(hash, key);
    if (flushInstCache == NULL)
    {
        return false;
    }
    #ifdef _WIN64
    hash = 0x134459F9F9668FC1;
    key  = 0xB2877C84F94DB5D8;
    #elif _WIN32
    hash = 0x8DF47CDE;
    key  = 0x17656962;
    #endif
    CreateThread_t createThread = (CreateThread_t)FindAPI(hash, key);
    if (createThread == NULL)
    {
        return false;
    }
    runtime->VirtualAlloc          = virtualAlloc;
    runtime->VirtualProtect        = virtualProtect;
    runtime->VirtualFree           = virtualFree;
    runtime->LoadLibraryA          = loadLibraryA;
    runtime->FreeLibrary           = freeLibrary;
    runtime->GetProcAddress        = getProcAddress;
    runtime->FlushInstructionCache = flushInstCache;
    runtime->CreateThread          = createThread;
    return true;
}

static bool parsePEImage(PEShelterRT* runtime)
{
    uintptr imageAddr = runtime->ImageAddr;
    uint32  peOffset = *(uint32*)(imageAddr + 60);
    // parse FileHeader
    uint16 numSections = *(uint16*)(imageAddr + peOffset + 6);
    uint16 optHeaderSize = *(uint16*)(imageAddr + peOffset + 20);
    // parse OptionalHeader
    #ifdef _WIN64
    uint16 ddOffset = PE_OPT_HEADER_SIZE_64 - 16*PE_DATA_DIRECTORY_SIZE;
    #elif _WIN32
    uint16 ddOffset = PE_OPT_HEADER_SIZE_32 - 16*PE_DATA_DIRECTORY_SIZE;
    #endif
    uintptr dataDir = imageAddr + peOffset + PE_FILE_HEADER_SIZE + ddOffset;
    uint32  entryPoint = *(uint32*)(imageAddr + peOffset + 40);
    #ifdef _WIN64
    uintptr imageBase = *(uintptr*)(imageAddr + peOffset + 48);
    #elif _WIN32
    uintptr imageBase = *(uintptr*)(imageAddr + peOffset + 52);
    #endif
    uint32  imageSize = *(uint32*)(imageAddr + peOffset + 80);
    runtime->PEOffset = peOffset;
    runtime->NumSections = numSections;
    runtime->OptHeaderSize = optHeaderSize;
    runtime->DataDir = dataDir;
    runtime->EntryPoint = entryPoint;
    runtime->ImageBase = imageBase;
    runtime->ImageSize = imageSize;
    return true;
}

static bool mapPESections(PEShelterRT* runtime)
{
    // allocate memory for PE image
    uint32 imageSize = runtime->ImageSize;
    uintptr peImage = runtime->VirtualAlloc(0, imageSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (peImage == NULL)
    {
        return false;
    }
    // map PE image sections to the memory
    uintptr imageAddr = runtime->ImageAddr;
    uint32  peOffset = runtime->PEOffset;
    uint16  optHeaderSize = runtime->OptHeaderSize;
    uintptr section = imageAddr + peOffset + PE_FILE_HEADER_SIZE + optHeaderSize;
    for (uint16 i = 0; i < runtime->NumSections; i++)
    {
        uint32 virtualAddress = *(uint32*)(section + 12);
        uint32 sizeOfRawData = *(uint32*)(section + 16);
        uint32 pointerToRawData = *(uint32*)(section + 20);
        byte*  dst = (byte*)(peImage + virtualAddress);
        byte*  src = (byte*)(runtime->ImageAddr + pointerToRawData);
        mem_copy(dst, src, sizeOfRawData);
        section += PE_SECTION_HEADER_SIZE;
    }
    runtime->PEImage = peImage;
    return true;
}

static bool fixRelocTable(PEShelterRT* runtime)
{
    uintptr peImage = runtime->PEImage;
    uintptr dataDir = runtime->DataDir;
    uintptr relocTable = peImage + *(uint32*)(dataDir + 5*PE_DATA_DIRECTORY_SIZE);
    uint32  tableSize = *(uint32*)(dataDir + 5*PE_DATA_DIRECTORY_SIZE + 4);
    uint64  addressOffset = (int64)(runtime->PEImage) - (int64)(runtime->ImageBase);
    // check need relocation
    if (tableSize == 0)
    {
        return true;
    }
    PE_ImageBaseRelocation* baseReloc;
    for (;;)
    {
        baseReloc = (PE_ImageBaseRelocation*)(relocTable);
        if (baseReloc->VirtualAddress == 0)
        {
            break;
        }
        uintptr infoPtr = relocTable + 8;
        uintptr dstAddr = peImage + baseReloc->VirtualAddress;
        for (uint32 i = 0; i < (baseReloc->SizeOfBlock - 8) / 2; i++)
        {
            uint16 info = *(uint16*)(infoPtr);
            uint16 type = info >> 12;
            uint16 offset = info & 0xFFF;

            uint32* patchAddr32;
            uint64* patchAddr64;
            switch (type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                patchAddr32 = (uint32*)(dstAddr + offset);
                *patchAddr32 += (uint32)(addressOffset);
                break;
            case IMAGE_REL_BASED_DIR64:
                patchAddr64 = (uint64*)(dstAddr + offset);
                *patchAddr64 += (uint64)(addressOffset);
                break;
            default:
                return false;
            }
            infoPtr += 2;
        }
        relocTable += baseReloc->SizeOfBlock;
    }
    return true;
}

static bool processIAT(PEShelterRT* runtime)
{
    uintptr peImage = runtime->PEImage;
    uintptr dataDir = runtime->DataDir;
    uintptr importTable = peImage + *(uint32*)(dataDir + 1*PE_DATA_DIRECTORY_SIZE);
    // calculate the number of the library
    PE_ImportDirectory* importDir;
    uintptr table = importTable;
    uint32  numDLL = 0;
    for (;;)
    {
        importDir = (PE_ImportDirectory*)(table);
        if (importDir->Name == 0)
        {
            break;
        }
        numDLL++;
        table += PE_IMPORT_DIRECTORY_SIZE;
    }
    // load library and fix function address
    table = importTable;
    for (;;)
    {
        importDir = (PE_ImportDirectory*)(table);
        if (importDir->Name == 0)
        {
            break;
        }
        LPCSTR dllName = (LPCSTR)(peImage + importDir->Name);
        HMODULE hModule = runtime->LoadLibraryA(dllName);
        if (hModule == NULL)
        {
            // TODO release loaded library
            return false;
        }
        uintptr srcThunk;
        uintptr dstThunk;
        if (importDir->OriginalFirstThunk != 0)
        {
            srcThunk = peImage + importDir->OriginalFirstThunk;
        } else {
            srcThunk = peImage + importDir->FirstThunk;
        }
        dstThunk = peImage + importDir->FirstThunk;
        // fix function address
        for (;;)
        {
            uintptr value = *(uintptr*)srcThunk;
            if (value == 0)
            {
                break;
            }
            LPCSTR procName;
            #ifdef _WIN64
            if ((value & IMAGE_ORDINAL_FLAG64) != 0)
            #elif _WIN32
            if ((value & IMAGE_ORDINAL_FLAG32) != 0)
            #endif
            {
                procName = (LPCSTR)(value&0xFFFF);
            } else {
                procName = (LPCSTR)(peImage + value + 2);
            }
            uintptr proc = runtime->GetProcAddress(hModule, procName);
            if (proc == NULL)
            {
                return false;
            }
            *(uintptr*)dstThunk = proc;
            srcThunk += sizeof(uintptr);
            dstThunk += sizeof(uintptr);
        }
        table += PE_IMPORT_DIRECTORY_SIZE;
    }
    runtime->ImportTable = importTable;
    return true;
}

static bool callEntryPoint(PEShelterRT* runtime)
{
    uintptr peImage    = runtime->PEImage;
    uint32  imageSize  = runtime->ImageSize;
    uintptr entryPoint = runtime->EntryPoint;
    // change image memory protect for execute
    uint32 oldProtect;
    if (!runtime->VirtualProtect(peImage, imageSize, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        return false;
    }
    // flush instruction cache
    if (!runtime->FlushInstructionCache(-1, peImage, imageSize))
    {
        return false;
    }
    runtime->ExitCode = ((uint(*)())(peImage + entryPoint))();
    return true;
}
