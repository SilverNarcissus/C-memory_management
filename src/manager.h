//
//  manager.h
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/8.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#ifndef manager_h
#define manager_h

//#include <stdio.h>
#include "const.h"

//**********************struct********************************

typedef struct{
    short is_full;
    short is_start;
    m_page_number before_page;
    m_page_number next_page;
}PAGE_ITEM;

typedef struct{
    short head;
    short rear;
    m_page_number start[CACHE_SIZE]; //4
    m_page_number page_offset[CACHE_SIZE]; //44
    m_page_number page_loc[CACHE_SIZE]; //84
}CACHE_BLOCK;

//**********************function********************************

//根据指定长度分配页表项
//length: 需要的页表数量
//return: 页表位图的起始点，如果页数不够，则返回0
unsigned int alloc_pages(int length);

//根据起始页号与偏移量寻找物理地址
p_address find_memory(m_page_number start, unsigned int offset, unsigned int total_size);

//顺序释放指定位置的页表项
int memory_free(m_page_number start_page);

//


#endif /* manager_h */




