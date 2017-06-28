//
//  my_lib.h
//  Memory_management
//
//  Created by Silver Narcissus on 2017/5/9.
//  Copyright © 2017年 Silver Narcissus. All rights reserved.
//

#ifndef my_lib_h
#define my_lib_h
#include "const.h"

void write_block(p_address des, data_unit* src, unsigned int size);
void read_block(data_unit* des, p_address src, unsigned int size);

#endif /* my_lib_h */
