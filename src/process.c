//
//  process.c
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/10.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#include "process.h"
#include "my_lib.h"
#include "manager.h"

static p_address get_pcb_loc(m_pid_t pid);
static p_address get_memory_item_loc(m_pid_t pid, short memory_item_loc);
static void write_memory_item(v_address v_add, m_size_t length
                              , unsigned int first_page_item_number,
                              m_pid_t pid, short memory_item_loc);
static unsigned int get_page_count(m_size_t length);
static short access_range_check(v_address start, m_size_t length, v_address target);
static int find_memory_item(v_address address, m_pid_t pid, MEMORY_ITEM* mem_item,
                            short (*checker)(v_address start, m_size_t length, v_address target));
static p_address find_p_add(v_address address, m_pid_t pid);
static short free_range_check(v_address start, m_size_t length, v_address target);


//给进程分配alloc项
//如果成功则返回分配的虚拟地址，否则返回-1
int process_alloc(m_pid_t pid, m_size_t length){
    //检查alloc项是否满
    short memory_item_number;
    read_block((data_unit*)&memory_item_number, get_pcb_loc(pid) + 2, 2);
    if (memory_item_number == MEMORY_ITEMS_SIZE){
        return -1;
    }
    //
    short max_to_scan;
    short memory_item_loc;
    read_block((data_unit*)&memory_item_loc, get_pcb_loc(pid), 2);
    read_block((data_unit*)&max_to_scan, get_pcb_loc(pid) + 4, 2);
    //第一次分配表项
    v_address last_add = 0;
    if (memory_item_loc == -1){
        write_block(get_pcb_loc(pid) + 8, (data_unit*)&last_add, 4);
    }
    else{
        read_block((data_unit*)&last_add, get_pcb_loc(pid) + 8, 4);
    }
    //检查分配的虚拟地址是否超出界限
    if (last_add + length > U32_MAX){
        return -1;
    }
    v_address new_add = last_add;
    last_add = last_add + length;
    unsigned int first_page_item_number = alloc_pages(get_page_count(length));
    if (first_page_item_number == -1){
        //如果内存数量不足
        return -1;
    }
    
    //test
    //unsigned int next = get_page_count(length);
    //read_block((char*)&next, 3413 * PAGE_ITEM_SIZE + 4, 4);
    //
    memory_item_number++;
    if (max_to_scan != MEMORY_ITEMS_SIZE){
        max_to_scan++;
        write_memory_item(new_add, length, first_page_item_number, pid, max_to_scan);
        memory_item_loc = max_to_scan;
        //更新max_to_scan
        write_block(get_pcb_loc(pid) + 4, (data_unit*)&max_to_scan, 2);
    }
    else{
        //寻找空的alloc项,并写入
        unsigned int flag = 1;
        for (int i = 0;i < MEMORY_ITEMS_SIZE; i++){
            read_block((data_unit*)&flag, get_memory_item_loc(pid, i) + 8, 4);
            if (flag == 0){
                write_memory_item(new_add, length, first_page_item_number, pid, i);
                memory_item_loc = i;
                break;
            }
        }
        //
    }
    
    //写回一些数据项
    write_block(get_pcb_loc(pid), (data_unit*)&memory_item_loc, 2);
    write_block(get_pcb_loc(pid) + 2, (data_unit*)&memory_item_number, 2);
    write_block(get_pcb_loc(pid) + 8, (data_unit*)&last_add, 4);
    return new_add;
}

//非法存取返回-1，成功返回0
int process_read(data_unit *data, v_address address, m_pid_t pid){
    p_address p_add = find_p_add(address, pid);
    if (p_add == 0){
        return -1;
    }
    //244318206
    read_block((data_unit*)data, p_add, 1);
    return 0;
}

//非法存取返回-1，成功返回0
int process_write(data_unit data, v_address address, m_pid_t pid){
    p_address p_add = find_p_add(address, pid);
    if (p_add == 0){
        return -1;
    }
    write_block(p_add, (data_unit*)&data, 1);
    return 0;
}

//释放指定内存区域
//释放成功返回0，非法释放返回-1;
int process_free(v_address address, m_pid_t pid){
    MEMORY_ITEM memory_item;
    short memory_item_loc = find_memory_item(address, pid, &memory_item, free_range_check);
    if (memory_item_loc == -1){
        return -1;
    }
    
    memory_free(memory_item.first_page_item_number);
    
    m_page_number free_number = 0;
    write_block(get_memory_item_loc(pid, memory_item_loc), (data_unit*)&free_number, 4);
    write_block(get_memory_item_loc(pid, memory_item_loc) + 4, (data_unit*)&free_number, 4);
    write_block(get_memory_item_loc(pid, memory_item_loc) + 8, (data_unit*)&free_number, 4);
    
    //更改段表可用数量
    short memory_item_number;
    read_block((data_unit*)&memory_item_number, get_pcb_loc(pid) + 2, 2);
    memory_item_number--;
    write_block(get_pcb_loc(pid) + 2, (data_unit*)&memory_item_number, 2);
    return 0;
}

//找到指定物理内存地址则返回物理地址，否则返回0
p_address find_p_add(v_address address, m_pid_t pid){
    MEMORY_ITEM memory_item;
    if (find_memory_item(address, pid, &memory_item, access_range_check) == -1){
        return 0;
    }
    return find_memory(memory_item.first_page_item_number, address - memory_item.v_add, memory_item.length);
}

//找到指定alloc项返回指定项下标，否则返回-1
int find_memory_item(v_address address, m_pid_t pid, MEMORY_ITEM* mem_item,
                     short (*checker)(v_address start, m_size_t length, v_address target)){
    //
    short max_to_scan;
    short memory_item_loc;
    read_block((data_unit*)&memory_item_loc, get_pcb_loc(pid), 2);
    read_block((data_unit*)&max_to_scan, get_pcb_loc(pid) + 4, 2);
    //
    v_address last_add = 0;
    if (memory_item_loc == -1){
        //进程未申请内存
        return -1;
    }
    else{
        read_block((data_unit*)&last_add, get_pcb_loc(pid) + 8, 4);
    }
    
    if (last_add <= address){
        //越上界
        return -1;
    }
    
    v_address now_start;
    m_size_t length;
    m_page_number start_page;
    
    //出于局部性原则，先读当前位置，再考虑遍历全部内存项
    read_block((data_unit*)&now_start, get_memory_item_loc(pid, memory_item_loc), 4);
    read_block((data_unit*)&length, get_memory_item_loc(pid, memory_item_loc) + 4, 4);
    if (checker(now_start, length, address) == TRUE){
        read_block((data_unit*)&start_page, get_memory_item_loc(pid, memory_item_loc) + 8, 4);
        mem_item->v_add = now_start;
        mem_item->length = length;
        mem_item->first_page_item_number = start_page;
        return memory_item_loc;
    }
    
    for (int i = 0;i <= max_to_scan; i++){
        read_block((data_unit*)&now_start, get_memory_item_loc(pid, i), 4);
        if (now_start == 0){
            continue;
        }
        read_block((data_unit*)&length, get_memory_item_loc(pid, i) + 4, 4);
        if (checker(now_start, length, address) == TRUE){
            read_block((data_unit*)&start_page, get_memory_item_loc(pid, i) + 8, 4);
            mem_item->v_add = now_start;
            mem_item->length = length;
            mem_item->first_page_item_number = start_page;
            return memory_item_loc;
        }
    }
    //未找到
    return -1;
}

//*************************下面是辅助函数**************************

//通过进程号获取进程PCB起始地址
p_address get_pcb_loc(m_pid_t pid){
    return (PROCESS_START) + pid * (PROCESS_SIZE);
}

//通过进程号和alloc项编号寻找alloc项
p_address get_memory_item_loc(m_pid_t pid, short memory_item_loc){
    return get_pcb_loc(pid) + (PROCESS_HEAD_SIZE) + memory_item_loc * (MEMORY_ITEM_SIZE);
}

//写入alloc项
void write_memory_item(v_address v_add, m_size_t length, unsigned int first_page_item_number,
                       m_pid_t pid, short memory_item_loc){
    p_address location = get_memory_item_loc(pid, memory_item_loc);
    write_block(location, (data_unit*)&v_add, 4);
    write_block(location + 4, (data_unit*)&length, 4);
    write_block(location + 8, (data_unit*)&first_page_item_number, 4);
}

//得到所需页表数量
unsigned int get_page_count(m_size_t length){
    unsigned int remain = 0;
    if (length % (PAGE_SIZE) != 0){
        remain = 1;
    }
    return length / (PAGE_SIZE) + remain;
}

//用于存取的checker
//如果在这个范围内，则返回TRUE
short access_range_check(v_address start, m_size_t length, v_address target){
    if ((target >= start)&&(target < (start + length))){
        return TRUE;
    }
    return FALSE;
}

//用于free的checker，必须保证起始地址一致
short free_range_check(v_address start, m_size_t length, v_address target){
    if (target == start){
        return TRUE;
    }
    return FALSE;
}

