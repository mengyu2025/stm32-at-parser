#ifndef __AT_PARSER_H
#define __AT_PARSER_H

#include "stm32f10x.h"
#include <stdio.h>  // 必须加，支持FILE/printf

// AT解析器状态枚举
typedef enum {
    AT_STATE_IDLE,        // 空闲态：等待接收'A'
    AT_STATE_RECV_T,      // 已收'A'，等待'T'
    AT_STATE_RECV_BODY,   // 已收'AT'，接收指令体
    AT_STATE_RECV_CR,     // 已收指令体，等待'\r'
    AT_STATE_RECV_LF,     // 已收'\r'，等待'\n'（指令完成）
    AT_STATE_ERROR        // 错误态：重置
} AT_Parser_State;

// 解析器结构体
typedef struct {
    AT_Parser_State state;
    char cmd_buf[64];
    uint8_t buf_len;
} AT_Parser_HandleTypeDef;

// 函数声明
void AT_Parser_Reset(AT_Parser_HandleTypeDef *parser);
void AT_Parser_Process_Char(AT_Parser_HandleTypeDef *parser, char ch);
void AT_Command_Handle(char *cmd);
void USART1_Init(u32 bound); // 串口初始化

#endif
