//
//  call.c
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/8.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#include "call.h"
#include "bottom.h"
#include "my_lib.h"
#include "manager.h"
#include "process.h"

//
//void init();
//int read(data_unit *data, v_address address, m_pid_t pid);
//int write(data_unit data, v_address address, m_pid_t pid);
//int allocate(v_address *address, m_size_t size, m_pid_t pid);
//int free(v_address address, m_pid_t pid);

void init(){
    //******************************
    //******初始化内存头部区间*********
    //******************************
    //初始化存储指针
    unsigned int m_page_table_loc = OS_RESERVE + 1;
    unsigned short m_page_available_number = (MEMORY_PAGE_NUMBER) - (OS_RESERVE) - 1;
    unsigned int p_page_table_loc = 0;
    unsigned int p_page_available_number = (TOTAL_PAGE_NUMBER) - (MEMORY_PAGE_NUMBER);
    unsigned short page_fault_position = OS_RESERVE + 1;
    unsigned short swap_position = OS_RESERVE;
    unsigned short cache_head = 0;
    unsigned short cache_rear = 0;
    
    write_block(GLOBAL_VAR_START, (data_unit*)&m_page_table_loc, 4);
    write_block(GLOBAL_VAR_START + 4, (data_unit*)&m_page_available_number, 2);
    
    write_block(GLOBAL_VAR_START + 6, (data_unit*)&p_page_table_loc, 4);
    write_block(GLOBAL_VAR_START + 10, (data_unit*)&p_page_available_number, 4);
    write_block(GLOBAL_VAR_START + 14, (data_unit*)&page_fault_position, 2);
    write_block(GLOBAL_VAR_START + 16, (data_unit*)&swap_position, 2);

    write_block(CACHE_START, (data_unit*)&cache_head, 2);
    write_block(CACHE_START + 2, (data_unit*)&cache_rear, 2);
//    unsigned int test1 = 0;
//    read_block((char*)&test1, GLOBAL_VAR_START + 6, 1);
    //初始化页表位图
    for (unsigned int i = 0; i < TOTAL_PAGE_NUMBER; i++){
        short is_full = 0;
        write_block(i * MEMORY_ITEM_SIZE, (data_unit*)&is_full, 2);
    }
    //******************************
    
    //******************************
    //******初始化进程区间*********
    //******************************
    for (unsigned int i = 0; i < PROCESS_NUMBER; i++){
        short memory_item_loc = -1;
        short memory_item_number = -1;
        short max_to_scan = -1;
        
        write_block(PROCESS_START + i * PROCESS_SIZE, (data_unit*)&memory_item_loc , 2);
        write_block(PROCESS_START + i * PROCESS_SIZE + 2, (data_unit*)&memory_item_number , 2);
        write_block(PROCESS_START + i * PROCESS_SIZE + 4, (data_unit*)&max_to_scan , 2);
    }

    //count_t test;
    //evaluate(&test, &test, &test, &test);
    
    //初始化一定的cache区域
    
}

int read(data_unit *data, v_address address, m_pid_t pid){
    return process_read(data, address, pid);
}

int write(data_unit data, v_address address, m_pid_t pid){
    return process_write(data, address, pid);;
}

int allocate(v_address *address, m_size_t size, m_pid_t pid){
    unsigned int result = process_alloc(pid, size);
    if (result == -1){
        return -1;
    }
    *(address) = result;
    return 0;
}

int free(v_address address, m_pid_t pid){
    return process_free(address, pid);
}

