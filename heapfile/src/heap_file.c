#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "heap_file.h"


#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

HP_ErrorCode HP_Init() {
  //insert code here
  return HP_OK;
}

HP_ErrorCode HP_CreateFile(const char *filename) {
  int fileDesc;
  BF_Block *initialBlock;

  BF_Block_Init(&initialBlock);
  //Creating a File and accessing its first empty block and initializing its components
  CALL_BF(BF_CreateFile(filename));
  CALL_BF(BF_OpenFile(filename, &fileDesc));
  CALL_BF(BF_AllocateBlock(fileDesc, initialBlock));

  char *data = BF_Block_GetData(initialBlock);
  *data = '1';  /* This character indicates that this file is a heap file */
  BF_Block_SetDirty(initialBlock);

  CALL_BF(BF_UnpinBlock(initialBlock));
  BF_Block_Destroy(&initialBlock);
  return HP_OK;
}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc){
  BF_Block *block;

  BF_Block_Init(&block);
  CALL_BF(BF_OpenFile(fileName, fileDesc));
  CALL_BF(BF_GetBlock(*fileDesc, 0, block));

  char *data = BF_Block_GetData(block);
  if (*data == '1') {
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    return HP_OK;
  } else {
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    return HP_ERROR;
  }
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  CALL_BF(BF_CloseFile(fileDesc));
  return HP_OK;
}

HP_ErrorCode HP_InsertEntry(int fileDesc, Record record){
  int numOfRecords;
  char *data, *startingPoint, *headerData;
  BF_Block *block;
  int blocks_num;

  // Number of Records that can fit in one Block
  numOfRecords = (BF_BLOCK_SIZE - HEADER_SIZE) / sizeof(record);

  CALL_BF(BF_GetBlockCounter(fileDesc, &blocks_num));
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(fileDesc, blocks_num - 1, block));

  headerData = BF_Block_GetData(block);   /*Number of Records that are currently in one Block*/
  if ((blocks_num == 1) || (*headerData) == numOfRecords) {  /* Edge Case */
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    BF_Block_Init(&block);
    CALL_BF(BF_AllocateBlock(fileDesc, block));

    headerData = BF_Block_GetData(block);
    *headerData = 0;
  }

  data = BF_Block_GetData(block);
  startingPoint = (data + HEADER_SIZE + (*headerData) * sizeof(record));
  memcpy(startingPoint, (char *)&record, sizeof(record));

  (*headerData)++;
  memcpy(data, headerData, sizeof(char));
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return HP_OK;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *attrName, void* value) {
  BF_Block *block;
  int blocks_num, i, j, recordNum;
  char * data;
  Record * record;
  Record dummy;

  CALL_BF(BF_GetBlockCounter(fileDesc, &blocks_num));
  for (i = 1; i < blocks_num; i++) {
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(fileDesc, i, block));
    data = BF_Block_GetData(block);
    recordNum = (*data);

    for (j = 0; j < recordNum; j++) {
      record = (Record *) (data + HEADER_SIZE + j * sizeof(dummy));

      if (value == NULL) {
        printf("Id: %d | Name: %s | Surname: %s | City: %s\n",record->id, record->name, record->surname, record->city);
        continue;
      }

       if(strcmp(attrName, "id") == 0) {
         if (record->id == *((int *) value)) {
           printf("Id: %d | Name: %s | Surname: %s | City: %s\n",record->id, record->name, record->surname, record->city);
         }
       } else if (strcmp(attrName, "name") == 0) {
         if (strcmp(record->name, value) == 0) {
           printf("Id: %d | Name: %s | Surname: %s | City: %s\n",record->id, record->name, record->surname, record->city);
         }
       } else if (strcmp(attrName, "surname") == 0) {
         if (strcmp(record->surname, value) == 0) {
           printf("Id: %d | Name: %s | Surname: %s | City: %s\n",record->id, record->name, record->surname, record->city);
         }
       } else if (strcmp(attrName, "city") == 0) {
         if(strcmp(record->city, value) == 0) {
           printf("Id: %d | Name: %s | Surname: %s | City: %s\n",record->id, record->name, record->surname, record->city);
         }
       }
    }
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
  }
  return HP_OK;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int rowId, Record *record) {
  BF_Block *block;
  char * data;
  int wantedBlock, wantedRecord, numOfRecords;
  Record dummy;
  char * recordPtr;

  numOfRecords = (BF_BLOCK_SIZE - HEADER_SIZE) / sizeof(dummy); /* How many records can fit in one block */

  wantedBlock = rowId / numOfRecords + 1;  /* The block that includes the wanted record */
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(fileDesc, wantedBlock, block));

  data = BF_Block_GetData(block);
  wantedRecord = rowId % numOfRecords;    /* Out of the (numOfRecords) records that are in the block, which one you want to return? */

  recordPtr = (data + HEADER_SIZE + wantedRecord * sizeof(dummy));
  memcpy(record, recordPtr, sizeof(dummy));

  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return HP_OK;
}
