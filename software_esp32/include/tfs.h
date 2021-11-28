//#ifndef __tfs_h_
//#define __tfs_h_

#pragma once

#include "stdint.h"

typedef struct tfs_dir_entry
{
    char*      NAME;
    uint32_t   FLAGS;
    uint8_t*   DATA;
    uint32_t   SIZE;
} TFS_DIR_ENTRY;


//#endif
