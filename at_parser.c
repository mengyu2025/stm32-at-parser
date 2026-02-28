#include "at_parser.h"
#include <string.h>

// 全局解析器实例
AT_Parser_HandleTypeDef at_parser = {.state = AT_STATE_IDLE, .buf_len = 0};

// 重置解析器
void AT_Parser_Reset(AT_Parser_HandleTypeDef *parser) {
    parser->state = AT_STATE_IDLE;
    parser->buf_len = 0;
    memset(parser->cmd_buf, 0, sizeof(parser->cmd_buf));
}

// 状态机处理单个字符（修改：放宽指令格式，兼容仅\r的情况）
void AT_Parser_Process_Char(AT_Parser_HandleTypeDef *parser, char ch) {
    switch (parser->state) {
        case AT_STATE_IDLE:
            if (ch == 'A' || ch == 'a') {
                parser->cmd_buf[parser->buf_len++] = ch;
                parser->state = AT_STATE_RECV_T;
            }
            break;
        case AT_STATE_RECV_T:
            if (ch == 'T' || ch == 't') {
                parser->cmd_buf[parser->buf_len++] = ch;
                parser->state = AT_STATE_RECV_BODY;
            } else {
                AT_Parser_Reset(parser);
            }
            break;
        case AT_STATE_RECV_BODY:
            // 关键修改：兼容仅收到\r的情况，直接触发处理（不用等\n）
            if (ch == '\r') {
                parser->cmd_buf[parser->buf_len++] = ch;
                parser->cmd_buf[parser->buf_len++] = '\n'; // 手动补\n，匹配strcmp判断
                AT_Command_Handle(parser->cmd_buf);
                AT_Parser_Reset(parser);
            } else if (ch == '\n') {
                parser->cmd_buf[parser->buf_len++] = ch;
                AT_Command_Handle(parser->cmd_buf);
                AT_Parser_Reset(parser);
            } else if (parser->buf_len < sizeof(parser->cmd_buf) - 1) {
                parser->cmd_buf[parser->buf_len++] = ch;
            } else {
                AT_Parser_Reset(parser);
            }
            break;
        case AT_STATE_RECV_CR: // 保留原逻辑，兼容完整\r\n
        case AT_STATE_RECV_LF:
        case AT_STATE_ERROR:
        default:
            AT_Parser_Reset(parser);
            break;
    }
}

// AT指令处理函数（可扩展）
void AT_Command_Handle(char *cmd) {
    if (strcmp(cmd, "AT\r\n") == 0) {
        // 基础AT指令，返回OK
        printf("OK\r\n");
    } else if (strstr(cmd, "AT+TEST")) {
        // 测试指令
        printf("OK: TEST\r\n");
    } else {
        // 未知指令
        printf("ERROR: Unknown CMD\r\n");
    }
}

// 串口1初始化（9600波特率）
void USART1_Init(u32 bound) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // 配置TX引脚（PA9）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置RX引脚（PA10）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 串口参数配置
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    // 开启串口中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // 配置中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 使能串口
    USART_Cmd(USART1, ENABLE);
}

// 重定向printf到串口1（关键修改：标志位判断错误！0X40是TC位，应改用TXE位0X80）
int fputc(int ch, FILE *f) {
    while ((USART1->SR & 0X80) == 0); // 修改：等待TXE（发送数据寄存器空），不是TC（发送完成）
    USART1->DR = (u8)ch;
    return ch;
}

// 串口1中断服务函数
void USART1_IRQHandler(void) {
    u8 res;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        res = USART_ReceiveData(USART1); // 读取接收的数据
        AT_Parser_Process_Char(&at_parser, res); // 交给状态机处理
    }
    USART_ClearITPendingBit(USART1, USART_IT_RXNE);
}
