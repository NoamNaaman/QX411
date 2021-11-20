#ifndef __SERIAL
#define __SERIAL


u32 get_comm_buffer_status(u32 channel);
u16 get_usart_byte(u32 channel);
void send_message(u32 ch, char *message);
void send_RS485_message(char *str);
void flush_rx_buffer(u32 channel);

#define USART_IN_BUF_LEN 128

extern u8 Rx2Buffer[USART_IN_BUF_LEN];   // low level serial input buffer
extern volatile u32 Rx2cnt, Rx2in, Rx2out; // low level input/output pointers and byte counter


#endif