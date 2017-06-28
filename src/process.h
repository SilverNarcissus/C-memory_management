//
//  process.h
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/10.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#ifndef process_h
#define process_h

#include "const.h"

//struct
typedef struct{
    v_address v_add;
    m_size_t length;
    m_page_number first_page_item_number;
}MEMORY_ITEM;

typedef struct{
    short memory_item_loc;
    short memory_item_number;
    short max_to_scan;
    short reserve;
    unsigned int last_v_add;
    MEMORY_ITEM memory_items[MEMORY_ITEMS_SIZE];
}PROCESS;

//function

//给进程分配alloc项
//如果成功则返回分配的虚拟地址，否则返回-1
int process_alloc(m_pid_t pid, m_size_t length);

//读指定进程的虚拟地址中的内容
//非法存取返回-1，成功返回0
int process_read(data_unit *data, v_address address, m_pid_t pid);

//写指定进程的虚拟地址中的内容
//非法存取返回-1，成功返回0
int process_write(data_unit data, v_address address, m_pid_t pid);

//释放指定进程的内存区域
//释放成功返回0，非法释放返回-1;
int process_free(v_address address, m_pid_t pid);

#endif /* process_h */
