#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "reg.h"
#include "threads.h"
extern int fibonacci(int x);
char *current_str;
char str_buf[64] = {0};
/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)
#define USART_FLAG_RXNE ((uint16_t) 0x0020)

void usart_init(void)
{
    *(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
    *(RCC_APB1ENR) |= (uint32_t) (0x00020000);

    /* USART2 Configuration, Rx->PA3, Tx->PA2 */
    *(GPIOA_CRL) = 0x00004B00;
    *(GPIOA_CRH) = 0x44444444;
    *(GPIOA_ODR) = 0x00000000;
    *(GPIOA_BSRR) = 0x00000000;
    *(GPIOA_BRR) = 0x00000000;

    *(USART2_CR1) = 0x0000000C;
    *(USART2_CR2) = 0x00000000;
    *(USART2_CR3) = 0x00000000;
    *(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
    while (*str) {
        while (!(*(USART2_SR) & USART_FLAG_TXE));
        *(USART2_DR) = (*str & 0xFF);
        str++;
    }
}

void print_char(const char *str)
{
    if (*str) {
        while (!(*(USART2_SR) & USART_FLAG_TXE));
        *(USART2_DR) = (*str & 0xFF);
    }
}

char recv_char(void)
{
    while (1) {
        if ((*USART2_SR) & (USART_FLAG_RXNE))
            return (*USART2_DR) & 0xff;
    }
}

void itoa(uint32_t value)//integer to string
{
    char temp[32] = {0};
    temp[31] = '\0';
    char *current_str = temp + 31;
    do {
        current_str -= 1;
        *current_str = value % 10 + 48;
        value /= 10;
    } while(value>0);
    print_str(current_str);
    print_str("\n");
}

void command_check(char *str)
{

    const char *tok = strtok(str," ");
    if(tok != NULL) {
        //if token "fib"
        if(tok[0] == 'f' && tok[1] == 'i' && tok[2] == 'b' && tok[3] == '\0') {
            tok = strtok(NULL," ");
            int value = atoi(tok);
            int res = fibonacci(value);
            itoa(res);
            while(tok != NULL)//token until end
                tok = strtok(NULL," ");
        } else {
            print_str("Error command. (Input fib + number)\n");
        }
    }
}

void my_shell()
{
    while(1) {
        print_str("my_shell@mini-arm-os> ");
        int index = 0;
        while(1) {
            str_buf[index] = recv_char();
            str_buf[index+1] = '\0';
            //"Enter"
            if(str_buf[index]==13 || str_buf[index] == '\n') {
                //Check command (fib)
                print_str("\n");
                command_check(str_buf);
                break;
            }
            //"backspace"
            else if(str_buf[index]==127) {
                if(index > 0) {
                    str_buf[index--] = '\0';
                    print_str("\b \b");
                }
            }
            //FIXME: maybe implement "Previous command"
            else {
                print_char(&str_buf[index]);
                index++;
            }
        }
    }
}

static void delay(volatile int count)
{
    count *= 50000;
    while (count--);
}

static void busy_loop(void *str)
{
    while (1) {
        print_str(str);
        print_str(": Running...\n");
        delay(1000);
    }
}

void test1(void *userdata)
{
    busy_loop(userdata);
}

void test2(void *userdata)
{
    busy_loop(userdata);
}

void test3(void *userdata)
{
    busy_loop(userdata);
}

/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
    const char *str1 = "Task1";

    usart_init();

    //Ready to test for shell
    if (thread_create(my_shell, (void *) str1) == -1)
        print_str("Thread 1 creation failed\r\n");

    /* SysTick configuration */
    *SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
    *SYSTICK_VAL = 0;
    *SYSTICK_CTRL = 0x07;

    thread_start();

    return 0;
}
