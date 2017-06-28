//
//  manager.c
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/8.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#include "manager.h"
#include "my_lib.h"
#include "bottom.h"
#define PAGE_FAULT_MOVE_DISTANCE 100
static short move(unsigned int* m_page_table_loc, short is_in_memory, unsigned int init_point);
static short page_fault_handler(m_page_number* page_location);
static void clean_cache();
static void refresh_cache(m_page_number start, unsigned int page_number, m_page_number page_location);
static p_address find_in_cache(m_page_number start, m_page_number page_number);
static void disable_error_cache(m_page_number start);

//length: 需要的页表数量
//return: 页表位图的起始点，如果页数不够，则返回0
unsigned int alloc_pages(int length){
    //先取出页表数量，判断是否能够alloc
    unsigned short m_page_available_number;
    unsigned int p_page_available_number;
    read_block((data_unit*)&m_page_available_number, GLOBAL_VAR_START + 4, 2);
    read_block((data_unit*)&p_page_available_number, GLOBAL_VAR_START + 10, 4);
    if((m_page_available_number + p_page_available_number) < length){
        return -1;
    }
    //读出cache所在位置
    unsigned short cache_position;
    read_block((data_unit*)&cache_position, GLOBAL_VAR_START + 16, 2);
    //将位图循环指针取出
    m_page_number m_page_table_loc;
    m_page_number init_point;
    unsigned int result = 0;
    short is_in_memory;
    if (m_page_available_number > 0){
        is_in_memory = TRUE;
        read_block((data_unit*)&m_page_table_loc, GLOBAL_VAR_START, 4);
    }
    else{
        is_in_memory = FALSE;
        read_block((data_unit*)&m_page_table_loc, GLOBAL_VAR_START + 6, 4);
    }
    init_point = m_page_table_loc;
    
    //保存使用循环链表的指针
    m_page_number first = 0;
    m_page_number before = 0;
    //
    for(int i = 0; i < length; i++){
        short is_full;
        read_block((data_unit*)&is_full, m_page_table_loc * PAGE_ITEM_SIZE, 2);
        
        if (is_full == 0 && m_page_table_loc != cache_position){
            is_full++;
            write_block(m_page_table_loc * PAGE_ITEM_SIZE, (data_unit*)&is_full, 2);
//            if (m_page_table_loc == 21333){
//                int x;
//                x=x+1;
//            }
            if (result == 0){
                //第一次分到页
                result = m_page_table_loc;
                first = m_page_table_loc;
                short is_first = TRUE;
                write_block(m_page_table_loc * PAGE_ITEM_SIZE + 2, (data_unit*)&is_first, 2);
            }
            //
            else{
                //写入上一个的next 和这一个的before
                write_block(before * PAGE_ITEM_SIZE + 8, (data_unit*)&m_page_table_loc, 4);
                write_block(m_page_table_loc * PAGE_ITEM_SIZE + 4, (data_unit*)&before, 4);
            }
            
            if (i == length - 1){
                //分到最后一个页
                //这里将最后一个页的next置为0，第一个的before置为最后一个
                //m_page_number last = 0;
                write_block(m_page_table_loc * PAGE_ITEM_SIZE + 8, (data_unit*)&first, 4);
                write_block(first * PAGE_ITEM_SIZE + 4, (data_unit*)&m_page_table_loc, 4);
            }
            
            before = m_page_table_loc;
            i++;
        }
        
        i--;
        is_in_memory = move(&m_page_table_loc, is_in_memory, init_point);
    }
    
    //写回页表指针和剩余数量
    if (is_in_memory){
        write_block(GLOBAL_VAR_START, (data_unit*)&m_page_table_loc , 4);
        m_page_available_number -= length;
        write_block(GLOBAL_VAR_START + 4, (data_unit*)&m_page_available_number, 2);
    }
    else{
        p_page_available_number = p_page_available_number - length + m_page_available_number;
        m_page_available_number = 0;
        write_block(GLOBAL_VAR_START + 6, (data_unit*)&m_page_table_loc, 4);
        write_block(GLOBAL_VAR_START + 4, (data_unit*)&m_page_available_number, 2);
        write_block(GLOBAL_VAR_START + 10, (data_unit*)&p_page_available_number, 4);
    }
    return result;
}

p_address find_memory(m_page_number start, unsigned int offset, unsigned int total_size){
    unsigned int page_number = offset / (PAGE_SIZE);
    unsigned int page_offset = offset % (PAGE_SIZE);
    //在cache中寻找
    p_address cache_page_start = find_in_cache(start, page_number);
    if (cache_page_start != 0){
        return cache_page_start + page_offset;
    }
    //
    unsigned int total_page_number = total_size / (PAGE_SIZE);
    m_page_number page_location = start;

    if (page_number > total_page_number / 2){
        //向前遍历
        for (unsigned int i= 0; i < total_page_number - page_number + 1; i++){
            read_block((data_unit*)&page_location, page_location * (PAGE_ITEM_SIZE) + 4, 4);
        }
    }
    else{
        //向后遍历
        for (unsigned int i= 0; i < page_number; i++){
            read_block((data_unit*)&page_location, page_location * (PAGE_ITEM_SIZE) + 8, 4);
        }
    }
    
    //count_t test;
    //evaluate(&test, &test, &test, &test);
    //更新cache
    //
    //处理缺页
    if (page_location >= MEMORY_PAGE_NUMBER){
        page_fault_handler(&page_location);
    }
    //更新cache
    refresh_cache(start, page_number, page_location);
    //
    return page_location * (PAGE_SIZE) + page_offset;
}

//顺序清空一段内存
int memory_free(m_page_number start_page){
    //清理cache
    disable_error_cache(start_page);
    //
    //取出总页数
    unsigned short m_page_available_number;
    unsigned int p_page_available_number;
    read_block((data_unit*)&p_page_available_number, GLOBAL_VAR_START + 10, 4);
    read_block((data_unit*)&m_page_available_number, GLOBAL_VAR_START + 4, 2);
    
    m_page_number next = start_page;
    short is_full;
    do{
        read_block((data_unit*)&is_full, next * (PAGE_ITEM_SIZE), 2);
        is_full--;
        //如果此页被清空
        if (is_full == 0){
            if(next >= MEMORY_PAGE_NUMBER){
                p_page_available_number++;
            }
            else{
                m_page_available_number++;
            }
        }
        //
        write_block(next * (PAGE_ITEM_SIZE), (data_unit*)&is_full, 2);
        
        read_block((data_unit*)&next, next * (PAGE_ITEM_SIZE) + 8, 4);
    } while(next != start_page);
    
    //写回总页数
    write_block(GLOBAL_VAR_START + 4, (data_unit*)&m_page_available_number, 2);
    write_block(GLOBAL_VAR_START + 10, (data_unit*)&p_page_available_number, 4);

    return 0;
}

//向前移动页表项指针
short move(unsigned int* m_page_table_loc, short is_in_memory, unsigned int init_point){
    unsigned int temp = *m_page_table_loc;
    temp++;
    if (is_in_memory){
        if (temp == MEMORY_PAGE_NUMBER){
            temp = OS_RESERVE;
        }
        if (temp == init_point){
            temp = MEMORY_PAGE_NUMBER;
            is_in_memory = FALSE;
        }
    }
    else{
        if (temp == TOTAL_PAGE_NUMBER){
            temp = MEMORY_PAGE_NUMBER;
        }
    }
    *m_page_table_loc = temp;
    return is_in_memory;
}

short page_fault_handler(m_page_number* page_location){
    m_page_number should_in = *(page_location);
    p_address disk_add = (should_in - (MEMORY_PAGE_NUMBER)) * (PAGE_SIZE);
    
    unsigned short page_fault_position;
    read_block((data_unit*)&page_fault_position, GLOBAL_VAR_START + 14, 4);
    
    
    short is_full;
    read_block((data_unit*)&is_full, page_fault_position * PAGE_ITEM_SIZE, 2);
    
    m_page_number disk_before;
    m_page_number disk_next;
    read_block((data_unit*)&disk_before, should_in * PAGE_ITEM_SIZE + 4, 4);
    read_block((data_unit*)&disk_next, should_in * PAGE_ITEM_SIZE + 8, 4);
    
    unsigned short swap_position;
    read_block((data_unit*)&swap_position, GLOBAL_VAR_START + 16, 2);
    unsigned int u32_swap_positon = swap_position;
    p_address memory_add = u32_swap_positon * (PAGE_SIZE);
    //返回值将是换入的页的内存位置
    m_page_number new_loc = u32_swap_positon;
    *(page_location) = new_loc;
    
    //如果当前位置是cache位置，则向前移动一下
    if (page_fault_position == swap_position){
        unsigned int temp = page_fault_position;
        move(&temp, TRUE, temp);
        page_fault_position = temp;
    }
    //如果当前位置是进程的第一页，则向前移动一下
    short is_first;
    read_block((data_unit*)&is_first, page_fault_position * PAGE_ITEM_SIZE + 2, 2);
    if (is_first == 1){
        unsigned int temp = page_fault_position;
        move(&temp, TRUE, temp);
        page_fault_position = temp;
    }
    //写入新的cache位置
    write_block(GLOBAL_VAR_START + 16, (data_unit*)&page_fault_position, 2);
    
    if (is_full == 0){
        //如果是空的页，则直接写入
        //只需要更换4个指针
        //需要减少内存页数，更换is_full信息
        //更换is_full信息
        is_full++;
        write_block(page_fault_position, (data_unit*)&is_full, 2);
        //减少内存页数
        unsigned short m_page_available_number;
        read_block((data_unit*)&m_page_available_number, GLOBAL_VAR_START + 4, 2);
        m_page_available_number--;
        write_block(GLOBAL_VAR_START + 4, (data_unit*)&m_page_available_number, 4);
        
        unsigned int temp = page_fault_position;
        move(&temp, TRUE, temp);
        page_fault_position = temp;
    }
    else{
        //如果不是空的页，则需要换入换出
        //由于是双向循环链表，需要更换8个指针
        m_page_number memory_before;
        m_page_number memory_next;
        read_block((data_unit*)&memory_before, page_fault_position * PAGE_ITEM_SIZE + 4, 4);
        read_block((data_unit*)&memory_next, page_fault_position * PAGE_ITEM_SIZE + 8, 4);
        
        write_block(should_in * PAGE_ITEM_SIZE + 4, (data_unit*)&memory_before, 4);
        write_block(should_in * PAGE_ITEM_SIZE + 8, (data_unit*)&memory_next, 4);
        write_block(memory_before * PAGE_ITEM_SIZE + 8, (data_unit*)&should_in, 4);
        write_block(memory_next * PAGE_ITEM_SIZE + 4, (data_unit*)&should_in, 4);

        unsigned int temp = page_fault_position;
        for (int i = 0; i < PAGE_FAULT_MOVE_DISTANCE; i++){
            move(&temp, TRUE, temp);
        }
        page_fault_position = temp;
    }
    
    write_block(u32_swap_positon * PAGE_ITEM_SIZE + 4, (data_unit*)&disk_before, 4);
    write_block(u32_swap_positon * PAGE_ITEM_SIZE + 8, (data_unit*)&disk_next, 4);
    write_block(disk_before * PAGE_ITEM_SIZE + 8, (data_unit*)&u32_swap_positon, 4);
    write_block(disk_next * PAGE_ITEM_SIZE + 4, (data_unit*)&u32_swap_positon, 4);
    
    disk_load(memory_add, disk_add, PAGE_SIZE);
    disk_save(page_fault_position * PAGE_SIZE, disk_add, PAGE_SIZE);
    
    //写入新的page_fault_position位置
    write_block(GLOBAL_VAR_START + 14, (data_unit*)&page_fault_position, 2);
    return 0;
}

//将cache清空
void clean_cache(){
    unsigned short cache_head = 0;
    unsigned short cache_rear = 0;
    write_block(CACHE_START, (data_unit*)&cache_head, 2);
    write_block(CACHE_SIZE, (data_unit*)&cache_rear, 2);
}

void refresh_cache(m_page_number start, unsigned int page_number, m_page_number page_location){
    short head;
    short rear;
    read_block((data_unit*)&head, CACHE_START, 2);
    read_block((data_unit*)&rear, CACHE_START + 2, 2);
    
    write_block(CACHE_START + 4 + rear * 4, (data_unit*)&start, 4);
    write_block(CACHE_START + 4 + CACHE_SIZE * 4 + rear * 4, (data_unit*)&page_number, 4);
    write_block(CACHE_START + 4 + CACHE_SIZE * 8 + rear * 4, (data_unit*)&page_location, 4);
    
    rear = (rear + 1) % (CACHE_SIZE);
    if (rear == head){
        head = (head + 1) % (CACHE_SIZE);
    }
    write_block(CACHE_START, (data_unit*)&head, 2);
    write_block(CACHE_START + 2, (data_unit*)&rear,  2);
}

//如果在cache中找到，则返回页的起始地址，否则返回0
p_address find_in_cache(m_page_number start, m_page_number page_number){
    short head;
    short rear;
    read_block((data_unit*)&head, CACHE_START, 2);
    read_block((data_unit*)&rear, CACHE_START + 2, 2);
    while (rear != head){
        if (rear == 0){
            rear = CACHE_SIZE - 1;
        }
        else{
            rear--;
        }
        unsigned int cache_start;
        read_block((data_unit*)&cache_start, CACHE_START + 4 + rear * 4, 4);
        if (cache_start == start) {
            unsigned int cache_offset;
            read_block((data_unit*)&cache_offset, CACHE_START + 4 + CACHE_SIZE * 4 + rear * 4, 4);
            if (page_number == cache_offset){
                m_page_number page_loc;
                read_block((data_unit*)&page_loc, CACHE_START + 4 + CACHE_SIZE * 8 + rear * 4, 4);
                return page_loc * (PAGE_SIZE);
            }
        }
    }

    return 0;
}

void disable_error_cache(m_page_number start){
    short head;
    short rear;
    read_block((data_unit*)&head, CACHE_START, 2);
    read_block((data_unit*)&rear, CACHE_START + 2, 2);
    while (rear != head){
        unsigned int cache_start;
        read_block((data_unit*)&cache_start, CACHE_START + 4 + rear * 4, 4);
        if (cache_start == start) {
            m_page_number disable = 0;
            write_block(CACHE_START + 4 + rear * 4, (data_unit*)&disable, 4);
            break;
        }
        if (rear == 0){
            rear = CACHE_SIZE - 1;
        }
        else{
            rear--;
        }
    }
}
