/*
 * This file is part of Module_Analyzer.

 * Module_Analyzer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Module_Analyzer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Module_Analyzer. If not, see <http://www.gnu.org/licenses/ .
 */

#include "../header/main.h"

#define SCE_INIT_APITYPE_MS2 0x141
#define SCE_INIT_APITYPE_EF2 0x152

PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_KERNEL, MOD_VER_MAJ, MOD_VER_MIN);

STMOD_HANDLER prev = NULL;

/*
 * Verifies if we're running a game or running homebrew by checking the ApiType.
 */
int isGame(void) {
	int api = sceKernelInitApitype();

	if (api == SCE_INIT_APITYPE_MS2 || api == SCE_INIT_APITYPE_EF2)
		return -1;

	memset(path_base, 0, sizeof(path_base));
	memset(gameId, 0, sizeof(gameId));

	strcpy(gameId, sceKernelGetGameInfo() + 0x44);
	sprintf(path_base, BASE_GAME_DIR_PATH, gameId);

	create_open_IoObject(IO_DIRECTORY, path_base, 0, 0777);

	return 0;
}

/* 
 * This only runs if the ApiType check in "isGame" fails. If it's gotten to this point, it must be homebrew.
 */
void isHomebrew(void) {
	memset(path_base, 0, sizeof(path_base));
	memset(path_brewName, 0, sizeof(path_brewName));

	strcpy(path_brewName, sceKernelInitFileName() + 0xE);
	stripTrail (path_brewName);
	sprintf(path_base, BASE_GAME_DIR_PATH, path_brewName);

	create_open_IoObject(IO_DIRECTORY, path_base, 0, 0777);
}

/**
 * Print attributes of a module: stub. segment location, entry address, global pointer value.
 * 
 * @param mod - Determining the module to receive the attributes from.
 * @param buf - Pointer to a buffer used for printing the attributes.
 */
static void print_mod_info(SceModule2 *mod, char *buf) {
    char tempBuf[MAX_INFO_LINE_LENGTH];
    int textLength;
    
    textLength = sprintf(tempBuf, STUB_SEGMENT_INFO_LINE, 
                         *(u32 *)mod->stub_top, (*(u32 *)mod->stub_top) + mod->stub_size);
    strncpy(buf, tempBuf, textLength); 
    
    textLength = sprintf(tempBuf, ENTRY_ADDRESS_INFO_LINE, mod->entry_addr);
    strncat(buf, tempBuf, textLength);
        
    textLength = sprintf(tempBuf, GLOBAL_POINTER_INFO_LINE, mod->gp_value);
    strncat(buf, tempBuf, textLength); 
}

/**
 * Analyze a module including attributes like its .text segment location, its .stub segment location
 * and its entry address.
 *
 * @param ioObjDesc - Opened file descriptor specifying the file to write the analyzed module data to. 
 * @param mod - Pointer to a SceModule2 struct of a module to analyze the above named aspects. 
 * @param modName - The name of the module which is currently being analyzed.
 */
void analyze_game_module(int ioObjDesc, SceModule2 *mod, const char *modName) {  
    char *modInfoBuf = NULL;
    int blockId;
    u32 impLibs = count_imported_libs(mod);
    u32 libNameLengths;
       
    LibStubSegment libSegment[impLibs];
                         
    modInfoBuf = (char *)allocate_memory_buffer(&blockId, MEM_PARTITION_USER, "Module_Info", PSP_SMEM_High, 
                                                MODULE_INFO_PRINT_LENGTH, NULL);                          
    if (modInfoBuf != NULL) {           
        print_mod_info(mod, modInfoBuf);        
    
        sceIoWrite(ioObjDesc, modInfoBuf, MODULE_INFO_PRINT_LENGTH);        
        sceKernelFreePartitionMemory(blockId);
    }
    else {
         sceIoWrite(ioObjDesc, MEMORY_ERROR_INFO, strlen(MEMORY_ERROR_INFO));
    }
    sceKernelFreePartitionMemory(blockId); //free allocated memory block
    
    libNameLengths = get_imported_libraries(mod, modName, libSegment);
    get_exported_libraries(mod, modName);
    
    map_segments(mod, modName, libSegment, impLibs, libNameLengths);  
}

/**
 * Analyze a module, whether it is a SONY OFW one or a custom one (i.e. Game/Homebrew).
 * Start module attributes analyzing.
 * 
 * @param mod - Pointer to a SceModule2 struct of a module which is currently being loaded. 
 *              This pointer will be passed to the function 'analyze_game_module' in order
 *              to analyze its attributes.
 */
static void get_module_list(SceModule2 *mod) {
    char modNameBuf[78];
    char modName[50];
    int ioObjDesc;
    static int ioOpenFileFlag = PSP_O_RDWR | PSP_O_CREAT | PSP_O_TRUNC;
    static u8 modCount = START_COUNTER_VAL;  
    char modNameLength;
    
    memset(modNameBuf, 0, sizeof(modNameBuf));
    memset(modName, 0, sizeof(modName));
	memset(path_base, 0, sizeof(path_base));
    
    strcpy(modName, mod->modname);
	if (gameId[0] != 0x00)
		sprintf(path_base, GAME_MODULE_FILE_PATH, gameId);
	else
		sprintf(path_base, GAME_MODULE_FILE_PATH, path_brewName);
	ioObjDesc = create_open_IoObject(IO_FILE, path_base, ioOpenFileFlag, 0777);
	if (IO_OBJECT_DESCRIPTOR_VALID(ioObjDesc)) {
        if (strncmp(modName, "sce", 3) == 0) {
            modNameLength = sprintf(modNameBuf, "Name of module_%02d: %s\n", modCount, modName);
            sceIoWrite(ioObjDesc, modNameBuf, modNameLength);
        }
        else {
             modNameLength = sprintf(modNameBuf, "Name of module_%02d: %s //Custom Module\n", modCount, modName);            
             sceIoWrite(ioObjDesc, modNameBuf, modNameLength);
             analyze_game_module(ioObjDesc, mod, modName);             
        }        
        sceIoClose(ioObjDesc);
        ioOpenFileFlag = PSP_O_RDWR | PSP_O_CREAT | PSP_O_APPEND;
        modCount++;       
    }
}

/*
 * Strips off the trailing ends from the buffer so we can make the directory.
 */
void stripTrail(char buf[]) {
	int i, offset;

	// Get the offset of '/'
	for (offset = PATH_LEN; offset > 0; offset--) {
		if (buf[offset] == '/') {
			break;
		}
	}

	// fill with 0's
	for (i = offset; i < PATH_LEN; i++)
		buf[i] = 0x00;
}

/**
 * Start the analyzing process of a module which is currently being loaded.
 */
static int on_module_start(SceModule2 *mod) {    
    get_module_list(mod);
    return (prev ? prev(mod) : 0);
}

/** 
 * Start memory_analyzer.prx and create/open the base directory and the base game directory
 * used by module_analyzer. Watch for other modules being loaded and set up their attributes analyzation. 
 */
int module_start(int args, void *argp) {
    create_open_IoObject(IO_DIRECTORY, BASE_DIR_PATH, 0, 0777);
	if (isGame() == -1)
		isHomebrew();
    
    prev = sctrlHENSetStartModuleHandler(on_module_start);    
    return 0;
}