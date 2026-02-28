#include "stm32f10x.h"
#include "at_parser.h"

int main(void) {
    // 初始化系统时钟（72MHz）
    SystemInit();
    // 初始化串口1（9600波特率）
    USART1_Init(9600);

    // 开机直接发送测试字符，验证串口发送功能
    printf("STM32 AT Parser Ready\r\n"); 
    printf("Send AT to test\r\n");

    while (1) {
        // 主循环空着，解析逻辑在中断里完成
    }
}
