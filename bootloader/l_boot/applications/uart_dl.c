#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_icu.h"
#include "driver_gpio.h"
#include "driver_uart1.h"
#include "driver_flash.h"


enum
{
    FLASH_ADDR_WRITE_CMD          = 0x06,
    FLASH_4K_WRITE_CMD         	 = 0x07,
    FLASH_ADDR_READ_CMD           = 0x08,
    FLASH_4K_READ_CMD     		 = 0x09,
    FLASH_CHIP_ERRASE_CMD       	 = 0x0A,
    FLASH_4K_ERRASE_CMD       	 = 0x0B,
    FLASH_SR_READ_CMD       		 = 0x0c,
    FLASH_SR_WRITE_CMD       	 = 0x0D,
    FLASH_SPI_OP_CMD      		 = 0x0E,
    FLASH_SIZE_ERRASE_CMD       	 = 0x0F,
};// FLASH_operate cmd

//other cmd
#define LINK_CHECK_CMD     0X00
#define CRC_CHECK_CMD      0X10
#define SET_RESET_CMD      0X0E
#define SET_BAUDRATE_CMD   0X0F
#define STAY_ROM_CMD       0XAA
#define BEKEN_UART_REGISTER_WRITE_CMD  0x01
#define BEKEN_UART_REGISTER_READ_CMD  0x03

enum
{
    UART_CMD_STATE_HEAD = 0,
    UART_CMD_STATE_OPCODE_ONE,
    UART_CMD_STATE_OPCODE_TWO,
    UART_CMD_STATE_LENGTH,
    UART_CMD_STATE_CMD,
    UART_CMD_STATE_CMD_FLASH,
    UART_CMD_STATE_LENGTH_FLASH_LEN0,
    UART_CMD_STATE_LENGTH_FLASH_LEN1,
    UART_CMD_STATE_LENGTH_FLASH_SCMD,
    UART_CMD_STATE_PAYLOAD,
    UART_CMD_STATE_ERROR_ONE,
    UART_CMD_STATE_ERROR_TWO,
    UART_CMD_STATE_ERROR_THREE,
    UART_CMD_STATE_ERROR_FOUR,
    UART_CMD_STATE_PACKET
};//cmd status



u8 bim_uart_cmd[16];
u8 bim_uart_data[4096 + 8];
u32 uart_download_status = 0;
u32 uart_buff_write = 0;
u8 bim_uart_rx_buf[512];
u32 erase_fenable = 0;
u32 crc32_table[256];

//#define     RBL_HEAD_LEN    96
//#define APP_RBL_BIN_ADDR            (RT_BK_APP_PART_END_ADDR/32*34 - RBL_HEAD_LEN - 6) //0x11AF94
u8 rbl_buf[106];
u32 uart_dl_port;

int make_crc32_table(void)
{
    u32 c;
    int i = 0;
    int bit = 0;
    for(i = 0; i < 256; i++)
    {
        c = (u32)i;
        for(bit = 0; bit < 8; bit++)
        {
            if(c & 1)
            {
                c = (c >> 1) ^ (0xEDB88320);
            }
            else
            {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
    return 0;
}
u32 make_crc32(u32 crc, unsigned char *string, u32 size)
{
    while(size--)
    {
        crc = (crc >> 8) ^ (crc32_table[(crc ^ *string++) & 0xff]);
    }
    return crc;
}


void  uart_download_rx(UINT8 val)
{
    bim_uart_rx_buf[uart_buff_write++] = (u8)val;
    if(uart_buff_write == 512)
        uart_buff_write = 0;
}


void dl_uart_init(unsigned long ulBaudRate)
{
    if(uart_dl_port == DEBUG_PORT_UART0)
    {
        uart0_init(ulBaudRate);
    }
    else if(uart_dl_port == DEBUG_PORT_UART1)
    {
        uart1_init(ulBaudRate);
    }
#if (CFG_SOC_NAME == SOC_BK7271)
    else if(uart_dl_port == DEBUG_PORT_UART2)
    {
        //uart2_init(ulBaudRate);
    }
#endif
}

void dl_uart_send(unsigned char *buff, int len)
{
    if(uart_dl_port == DEBUG_PORT_UART0)
    {
        uart0_send(buff, len);
    }
    else if(uart_dl_port == DEBUG_PORT_UART1)
    {
        uart1_send(buff, len);
    }
    #if (CFG_SOC_NAME == SOC_BK7271)
    else if(uart_dl_port == DEBUG_PORT_UART2)
    {
        //uart2_send(buff, len);
    }
    #endif
}

void cmd_response( u8 cmd, u8 length, u8 *payload )
{
    u8 response_buff[16], i;

    if(length < 4)
        return;

    response_buff[0] = 0x04;
    response_buff[1] = 0x0e;
    response_buff[2] = length;
    response_buff[3] = 0x01;
    response_buff[4] = 0xe0;
    response_buff[5] = 0xfc;
    response_buff[6] = cmd;

    for(i = 0; i < length - 4; i++)
        {
        response_buff[7 + i] = payload[i];
        }

    dl_uart_send(response_buff, length + 3);

}

void operate_flash_cmd_response( u8 cmd, u8 status, u16 length, u8 *payload )
{
    u8 response_buff[4200];
    u16    i;

    if(length < 2)
        return;

    response_buff[0] = 0x04;
    response_buff[1] = 0x0e;
    response_buff[2] = 0xff;
    response_buff[3] = 0x01;
    response_buff[4] = 0xe0;
    response_buff[5] = 0xfc;
    response_buff[6] = 0xf4;

    response_buff[7] = (length & 0xff);
    response_buff[8] = length >> 8;
    response_buff[9] = cmd;
    response_buff[10] = status;


    for(i = 0; i < (length - 2); i++)
        response_buff[11 + i] = payload[i];

    dl_uart_send(response_buff, length + 9);
}


void uart_cmd_dispath(u8 *buff, u8 len)
{
    u8 payload[16];
    u8 read_data[256];
    u32  calcuCrc = 0xffffffff;
    u32 cur_clk_cnt, delay_unit;
    u32 read_flash_addr;
    u32 uart_clk_div, baudrate_set;
    u32 crc_start_addr, crc_end_addr;
		uint16_t i;
#if 1
    //bk_printf("\nc:");
    //bk_print_hex(buff[0]);
    
    switch(buff[0])
    {
    case LINK_CHECK_CMD:
        uart_download_status = 1;
        payload[0] = 0x00;
        cmd_response(LINK_CHECK_CMD + 1, 5, payload);
        erase_fenable = 1;

        break;

    case CRC_CHECK_CMD:
        crc_start_addr = ( buff[1] | (buff[2] << 8) | (buff[3] << 16) | (buff[4] << 24) );
        crc_end_addr = ( buff[5] | (buff[6] << 8) | (buff[7] << 16) | (buff[8] << 24) );
        //set_flash_protect(1);

        make_crc32_table();
        read_flash_addr = crc_start_addr;

        for(i = 0; i < (crc_end_addr - crc_start_addr + 1) / 256; i++)
        {
            flash_read_data(read_data, read_flash_addr, 256);
            calcuCrc = make_crc32(calcuCrc, read_data, 256);
            read_flash_addr += 256;
        }

        /*write app.bin rbl*/
        //flash_erase_sector((UINT32)APP_RBL_BIN_ADDR & ~((UINT32)0xFFF));
        //flash_read_data(rbl_buf, crc_end_addr + 1 - 102, 102);
        //flash_write_data(&rbl_buf, (UINT32)APP_RBL_BIN_ADDR, 102);
        set_flash_protect(ALL);

        payload[0] = calcuCrc;
        payload[1] = calcuCrc >> 8;
        payload[2] = calcuCrc >> 16;
        payload[3] = calcuCrc >> 24;
        cmd_response(CRC_CHECK_CMD, 8, payload);
        break;

    case SET_BAUDRATE_CMD:
#if 1
        baudrate_set =  buff[1] | (buff[2] << 8) | (buff[3] << 16) | (buff[4] << 24) ;
        delay_unit = (buff[5] + 1); //2ms unit
        cur_clk_cnt = fclk_get_tick();
        dl_uart_init(baudrate_set);

        while(fclk_get_tick() < delay_unit + cur_clk_cnt);

        payload[0] = buff[1];
        payload[1] = buff[2];
        payload[2] = buff[3];
        payload[3] = buff[4];
        payload[4] = buff[5];
        cmd_response(SET_BAUDRATE_CMD, 9, payload);
        break;

    case SET_RESET_CMD:
        if(buff[1] == 0xa5)
        {
            //bk_printf("wdt_reboot\r\n");
            DelayMS(1);
            sys_forbidden_interrupts();
            *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_IN_BOOT_RESET2_VALUE;
            wdt_reboot();
            while(1);
        }
#endif
        
#if 0
    case BEKEN_UART_REGISTER_WRITE_CMD:
        crc_start_addr = ( buff[1] | (buff[2] << 8) | (buff[3] << 16) | (buff[4] << 24) );
        crc_end_addr = ( buff[5] | (buff[6] << 8) | (buff[7] << 16) | (buff[8] << 24) );

        REG_WRITE(crc_start_addr, crc_end_addr);
        //cmd_response(BEKEN_UART_REGISTER_READ_CMD, 8, payload);
        break;

    case BEKEN_UART_REGISTER_READ_CMD:
        crc_start_addr = ( buff[1] | (buff[2] << 8) | (buff[3] << 16) | (buff[4] << 24) );
    
        calcuCrc = REG_READ(crc_start_addr);

        payload[0] = buff[1];
        payload[1] = buff[2];
        payload[2] = buff[3];
        payload[3] = buff[4];
 
        payload[4] = calcuCrc;
        payload[5] = calcuCrc >> 8;
        payload[6] = calcuCrc >> 16;
        payload[7] = calcuCrc >> 24;
        cmd_response(BEKEN_UART_REGISTER_READ_CMD, 12, payload);
        break;
#endif

    }
#endif
}


void boot_uart_data_callback( u8 *buff, u16 len)
{
    static u8 cmd_status = UART_CMD_STATE_HEAD;
    static u16 index = 0, index_cnt = 0;
    static u16 length;
    static u16 scmd_length;
    static u32 write_addr;
    static u32 read_addr;
    static u32 read_buff[256];
    
    while(len > 0)
    {

        switch(cmd_status)
        {
        case UART_CMD_STATE_HEAD:
        {
            if(buff[0] == 0x01)
            {
                cmd_status = UART_CMD_STATE_OPCODE_ONE;
            }
            else
                cmd_status = UART_CMD_STATE_HEAD;
        }
        break;
        case UART_CMD_STATE_OPCODE_ONE:
        {
            if( buff[0] == 0xe0 )
                cmd_status = UART_CMD_STATE_OPCODE_TWO;
            else
                cmd_status = UART_CMD_STATE_HEAD;
        }
        break;
        case UART_CMD_STATE_OPCODE_TWO:
        {
            if( buff[0] == 0xfc )
                cmd_status = UART_CMD_STATE_LENGTH;
            else
                cmd_status = UART_CMD_STATE_HEAD;
        }
        break;
        case UART_CMD_STATE_LENGTH:
        {
            length = buff[0];

            if(0xff == buff[0])
            {
                cmd_status = UART_CMD_STATE_CMD_FLASH;
            }
            else if( buff[0] > 0 && buff[0] != 0xff )
            {
                cmd_status = UART_CMD_STATE_CMD;
                index = 0;
            }
            else
                cmd_status = UART_CMD_STATE_HEAD;
        }
        break;

        case UART_CMD_STATE_CMD:
        {
            bim_uart_cmd[index++] = buff[0];

            if(index == length)
            {
                uart_cmd_dispath(bim_uart_cmd, length);
                cmd_status = UART_CMD_STATE_HEAD;
            }
        }
        break;

        case UART_CMD_STATE_CMD_FLASH:
        {
            if( buff[0] == 0xf4 )
                cmd_status = UART_CMD_STATE_LENGTH_FLASH_LEN0;
            else
                cmd_status = UART_CMD_STATE_HEAD;
        }
        break;
        case UART_CMD_STATE_LENGTH_FLASH_LEN0:
        {
            cmd_status = UART_CMD_STATE_LENGTH_FLASH_LEN1;
            scmd_length = buff[0];

        }
        break;
        case UART_CMD_STATE_LENGTH_FLASH_LEN1:
        {

            scmd_length += (buff[0] << 8);

            if(scmd_length > 0)
                cmd_status = UART_CMD_STATE_LENGTH_FLASH_SCMD;
            else
                cmd_status = UART_CMD_STATE_HEAD;

            index = 0;
            index_cnt = 0;
        }
        break;

        case UART_CMD_STATE_LENGTH_FLASH_SCMD:
        {
            bim_uart_data[index++] = buff[0];
            if(bim_uart_data[0] == FLASH_4K_READ_CMD && index == scmd_length) //read id
            {
                read_addr = bim_uart_data[1] | (bim_uart_data[2] << 8) | (bim_uart_data[3] << 16) | (bim_uart_data[4] << 24) ;

				if(read_addr < UART_DL_LIMIT_ADDR)
                {
                    bim_uart_data[5] = scmd_length - 5;
                    operate_flash_cmd_response(FLASH_4K_READ_CMD, 6, 7, &bim_uart_data[1]);
                }
                else
                {
                    flash_read_data(&bim_uart_data[5], read_addr, 4096);
                    operate_flash_cmd_response(FLASH_4K_READ_CMD, 0, 4102, &bim_uart_data[1]);

                }
                cmd_status = UART_CMD_STATE_HEAD;

            }
            else if(bim_uart_data[0] == FLASH_SIZE_ERRASE_CMD && index == scmd_length) //erase
            {
                INT32 addr = bim_uart_data[2] | (bim_uart_data[3] << 8) | (bim_uart_data[4] << 16) | (bim_uart_data[5] << 24) ;
                if(addr < UART_DL_LIMIT_ADDR)
                {
                    operate_flash_cmd_response(0x0f, 6, 0x07, &bim_uart_data[1]);
                }
                else
                {
                    if(erase_fenable == 1)
                    {
                        set_flash_protect(NONE);
                        erase_fenable = 0;
                    }

                    if(bim_uart_data[1] == 0x20)
                    {
                        flash_erase_sector(addr);
                    }
                    else if(bim_uart_data[1] == 0xd8)
                    {
                        flash_erase_block(addr);
                    }

                    operate_flash_cmd_response(FLASH_SIZE_ERRASE_CMD, 0, 0x07, &bim_uart_data[1]);
                }
                cmd_status = UART_CMD_STATE_HEAD;
            }
            else if(bim_uart_data[0] == FLASH_ADDR_WRITE_CMD && index == scmd_length) //write bt addr
            {
                write_addr = bim_uart_data[1] | (bim_uart_data[2] << 8) | (bim_uart_data[3] << 16) | (bim_uart_data[4] << 24) ;

                if(write_addr < UART_DL_LIMIT_ADDR)
                {
                    bim_uart_data[5] = scmd_length - 5;
                    operate_flash_cmd_response(FLASH_ADDR_WRITE_CMD, 6, 7, &bim_uart_data[1]);
                }
                else
                {
                    flash_write_data(&bim_uart_data[5], write_addr, (scmd_length - 5));
                    bim_uart_data[5] = scmd_length - 5;
                    operate_flash_cmd_response(FLASH_ADDR_WRITE_CMD, 0, 7, &bim_uart_data[1]);
                }
                cmd_status = UART_CMD_STATE_HEAD;
            }
            else if( bim_uart_data[0] == FLASH_4K_WRITE_CMD ) //write code data
            {
                if(index >= (256 * (index_cnt + 1) + 5) )
                {
                    write_addr = bim_uart_data[1] | (bim_uart_data[2] << 8) | (bim_uart_data[3] << 16) | (bim_uart_data[4] << 24) ;
                    if(write_addr < 0x2000)
                    {
                        index_cnt++;
                        if(index == scmd_length )
                        {
                            bim_uart_data[1] = 0x00;
                            bim_uart_data[2] = 0x20;
                            operate_flash_cmd_response(FLASH_4K_WRITE_CMD, 6, 6, &bim_uart_data[1]);
                            cmd_status = UART_CMD_STATE_HEAD;
                            index_cnt = 0;
                        }
                    }
                    else
                    {
                        flash_write_data(&bim_uart_data[5 + 256 * (index_cnt)], (write_addr + 256 * (index_cnt)), 256);
                        index_cnt++;
                        if(index == scmd_length )
                        {
                            operate_flash_cmd_response(FLASH_4K_WRITE_CMD, 0, 6, &bim_uart_data[1]);
                            cmd_status = UART_CMD_STATE_HEAD;
                            index_cnt = 0;
                        }
                    }
                }
            }
            else if(index == scmd_length)
                cmd_status = UART_CMD_STATE_HEAD;
        }
        break;
        }
        len--;
        buff++;
    }
}



