//
//  my_lib.c
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/9.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#include "my_lib.h"
#include "bottom.h"

void write_block(p_address des, data_unit* src, unsigned int size){
    data_unit* temp = src;
    for (int i = 0;i < size; i++){
        mem_write(*temp, des);
        des++;
        temp++;
    }
}

void read_block(data_unit* des, p_address src, unsigned int size){
    data_unit* temp = des;
    for (int i = 0;i < size; i++){
        *temp = mem_read(src);
        src++;
        temp++;
    }
}

