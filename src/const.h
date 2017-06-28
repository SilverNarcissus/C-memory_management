//
//  const.h
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/10.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#ifndef const_h
#define const_h

#include "type.h"

#define PAGE_SIZE 4 * 1024
#define MEMORY_ITEMS_SIZE 1000
#define CACHE_SIZE 10
#define MEMORY_PAGE_NUMBER (1024 * 1024 * 128) / (PAGE_SIZE)
#define TOTAL_PAGE_NUMBER (1024 * 1024 * (128 + 512) ) / (PAGE_SIZE)
//struct size
#define PAGE_ITEM_SIZE 12
#define MEMORY_ITEM_SIZE 12
#define PROCESS_SIZE 12012
#define PROCESS_HEAD_SIZE 12
//
#define PROCESS_NUMBER 1000
#define OS_RESERVE (PROCESS_SIZE * PROCESS_NUMBER + TOTAL_PAGE_NUMBER * PAGE_ITEM_SIZE)\
/ (PAGE_SIZE) + 1
#define PROCESS_START    (TOTAL_PAGE_NUMBER) * (PAGE_ITEM_SIZE)
#define GLOBAL_VAR_START PROCESS_SIZE * PROCESS_NUMBER + (TOTAL_PAGE_NUMBER) * PAGE_ITEM_SIZE
#define CACHE_START (GLOBAL_VAR_START) + 18
#define GLOBAL_VAR_SIZE  (OS_RESERVE) * (PAGE_SIZE) - (GLOBAL_VAR_START)

#define U32_MAX 4093640703

#define FALSE 0
#define TRUE 1

typedef unsigned int m_page_number;
typedef unsigned int m_pointer;

#endif /* const_h */
