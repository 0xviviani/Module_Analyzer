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

#include "../header/stubs.h"
#include "../header/main.h"

void map_imported_functions(int *ioOpenFlag, SceLibraryStubTable *stubTable, const char *modName, LibStubSegment *libSegPtr, int *libNameLengths) {
    char *libImportsBuf = NULL;
    char buf[128];
    int ioObjDesc;
    int blockId;
    u32 textLength = 0;
    u32 stubs;   
    u32 libImpBufSize; 
    
    memset(buf, 0, sizeof(buf));    
	memset(path_base, 0, sizeof(path_base));
	if (gameId[0] != 0x00)
		sprintf(path_base, MODULE_LIB_IMPORTS_FILE_PATH, gameId, modName);
	else
		sprintf(path_base, MODULE_LIB_IMPORTS_FILE_PATH, path_brewName,
				modName);
	ioObjDesc = create_open_IoObject(IO_FILE, path_base, *ioOpenFlag, 0777);

	stubs = stubTable->stubcount;
	*libNameLengths += get_importedLibrary_segAttr(stubTable, libSegPtr, stubs);
               
    if (IO_OBJECT_DESCRIPTOR_VALID(ioObjDesc)) {                       
        textLength = sprintf(buf, "-Imported NIDs from library: %s\n", stubTable->libname);
        libImpBufSize = textLength + IMPORTED_NID_INFO_LINE_LENGTH * stubs;
        
        libImportsBuf = (char *)allocate_memory_buffer(&blockId, MEM_PARTITION_USER, "library_imports", 
                                                       PSP_SMEM_High, libImpBufSize, NULL);      
        
        if (libImportsBuf != NULL) {           
            strncpy(libImportsBuf, buf, libImpBufSize);
            
            int i;
            for (i = 0; i < stubs; i++) {
                 textLength = sprintf(buf, IMPORTED_NID_INFO_LINE, i + 1, stubTable->nidtable[i], 
                                      (u32)(stubTable->stubtable + i * STUB_SIZE));
                 strncat(libImportsBuf, buf, textLength);
            } 
            sceIoWrite(ioObjDesc, libImportsBuf, libImpBufSize);
            sceKernelFreePartitionMemory(blockId);   
        }
        else {
             sceIoWrite(ioObjDesc, MEMORY_ERROR_INFO, strlen(MEMORY_ERROR_INFO));
        }
    }      
    sceIoClose(ioObjDesc);
    *ioOpenFlag = PSP_O_RDWR | PSP_O_CREAT | PSP_O_APPEND;
}