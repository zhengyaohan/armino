/*************************************************************
 * @file        data_move.c
 * @brief       data move of ota
 * @author      wifi team
 * @version     V1.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "BK_System.h"
#include "data_move.h"
#include "tiny_aes.h"
#include "zlib.h"
#include "driver_flash.h"
#include "fal_def.h"
#include "fal_cfg.h"
#include "drv_uart.h"
#include "fal.h"

/* buffer use information:
	1, read flash data len:DM_CHUNK, in_buf
	2, aes decrypt len:DM_CHUNK, out_buf
	3, zlib decompress len:DM_CHUNK, in_buf
	4, flash write len: DM_CHUNK * 34 / 32, flash_buf
 */
uint32_t in_buf[DM_CHUNK >> 2];
uint32_t out_buf[DM_CHUNK >> 2];
uint32_t flash_buf[FLASH_WR_CHUNK >> 2];
uint32_t pre_encrypt_buf[FLASH_WR_CHUNK >> 2]; /* efuse encrypt buffer*/
uint8_t remnant[INS_NO_CRC_CHUNK];
uint8_t remnantx[INSTRUCTION_CRC_CHUNK]={0};
uint32_t remnant_len = 0;
uint32_t src_offset1=0;
extern void encrypt(u32 *rx, u8 *tx, u32 num, u32  addr0);
extern void calc_crc(u32 *buf, u32 packet_num);

void dump_hex(const uint8_t *ptr, size_t buflen)
{
	unsigned char *buf = (unsigned char*)ptr;
	int i, j;

	for (i=0; i<buflen; i+=16)
	{
		printf("%08X: ", i);

		for (j=0; j<16; j++)
			if (i+j < buflen)
				printf("%02X ", buf[i+j]);
			else
				printf("   ");
		printf(" ");

		for (j=0; j<16; j++)
			if (i+j < buflen)
				printf("%c", __is_print(buf[i+j]) ? buf[i+j] : '.');
		printf("\n");
	}
}

int aes_decrypt_handler(char *in, char *out, int count)
{
    int len;
    char *buf, *data_decrypt;
    tiny_aes_context ctx;
    uint8_t iv[TINY_AES_IV_LEN + 1];
    uint8_t private_key[TINY_AES_KEY_LEN + 1];

	log_i("in:%p, out:%p, count:%d", in, out, count);

    buf = in;
    len = count;
    data_decrypt = out;

    memcpy(iv, BK_TINY_AES_IV, strlen(BK_TINY_AES_IV));
    iv[sizeof(iv) - 1] = '\0';
    memcpy(private_key, BK_TINY_AES_KEY, strlen(BK_TINY_AES_KEY));
    private_key[sizeof(private_key) - 1] = '\0';

    memset(data_decrypt, 0x0, len);
    tiny_aes_setkey_dec(&ctx, (uint8_t *) private_key, 256);
    tiny_aes_crypt_cbc(&ctx, AES_DECRYPT, len, iv, (unsigned char *)buf, (unsigned char *)data_decrypt);

    return RET_DM_SUCCESS;
}

int data_move_start(fal_partition_t part)
{
#define RD_COUNT    (32)

    int i, ret = 0;
    int count = RD_COUNT;
    int data_0xff_cnt = 0;
    unsigned char rd[RD_COUNT] = {0};

	bk_printf("data_move_start");

    flash_read_data(rd, part->offset, RD_COUNT);
    for(i = 0; i < count; i ++)
    {
        if (0xff == rd[i])
        {
            data_0xff_cnt ++;
        }
		if(0 == i%8)
			bk_printf("\r\n ");
		bk_print_hex(rd[i]);
    }

    if(count == data_0xff_cnt)
    {
        ret = 1;
    }

    return ret;
}

int data_move_end(uint32_t addr)
{
	log_i("data_move_end:%d", DL_SEC_BASE);
    flash_erase_sector(addr);
	
    return 0;
}

int dm_erase_dest_partition(uint32_t addr, uint32_t len)
{
    ASSERT(0 == (addr & (0x1000 - 1)));
    ASSERT(0 == (len & (0x1000 - 1)));

	log_i("dm_erase_dest_partition:0x%x, %d", addr, len);

    flash_erase(addr, len);
	
    return 0;
}

int dm_rd_src_partition(uint32_t flash_addr, char *buf, int len)
{
	log_i("flash_rd:0x%x,:0x%x, %d", flash_addr, buf, len);
    flash_read_data((unsigned char *)buf, flash_addr, len);

    return len;
}

int dm_stay_unalignment_remant(void *unalign_ptr, uint32_t len)
{
	ASSERT(INS_NO_CRC_CHUNK > len);
	
	memcpy(remnant, unalign_ptr, len);
	remnant_len = len;

	return len;
}

int dm_efuse_enc_buf_convert(uint8_t *remnant_ptr, uint32_t remnant_cnt, void *in, void *out, uint32_t count)
{
	int ret;
	int cell_cnt, i, j;
	char *no_crc_cell;
	char *crc_cell;
	char *input_ptr;
	char *output_ptr;
	uint32_t padding_cnt;
	uint32_t handle_all_cnt;

	handle_all_cnt = remnant_cnt + count;
	ASSERT(0 == (handle_all_cnt & (INS_NO_CRC_CHUNK - 1)));
	cell_cnt = handle_all_cnt / INS_NO_CRC_CHUNK;
	ret = cell_cnt;

	output_ptr = (char *)out;
	input_ptr = (char *)in;
	if(remnant_cnt)
	{
		padding_cnt = INS_NO_CRC_CHUNK - remnant_cnt;
		
		memcpy(output_ptr, remnant_ptr, remnant_cnt);
		memcpy(&output_ptr[remnant_cnt], input_ptr, padding_cnt);
		
		output_ptr[CRC_BYTE0_OFFSET] = 0xFF;
		output_ptr[CRC_BYTE1_OFFSET] = 0xFF;

		/* update in/out pointer*/
		input_ptr = &input_ptr[padding_cnt];
		output_ptr = &output_ptr[INSTRUCTION_CRC_CHUNK];
		cell_cnt -= 1;
	}

	no_crc_cell = (char *)input_ptr;
	crc_cell = (char *)output_ptr;
	for(i = 0; i < cell_cnt; i ++)
	{
		for(j = 0; j < INS_NO_CRC_CHUNK; j ++)
		{
			crc_cell[j] = no_crc_cell[j];
		}
		
		crc_cell[CRC_BYTE0_OFFSET] = 0xFF;
		crc_cell[CRC_BYTE1_OFFSET] = 0xFF;

		no_crc_cell = (char *)((uint32_t)no_crc_cell + INS_NO_CRC_CHUNK);
		crc_cell = (char *)((uint32_t)crc_cell + INSTRUCTION_CRC_CHUNK);
	}

	return ret;
}

/*
	paramter:
		addr: physical flash address;
		ptr: write data pointer
		size: cell length
		nmemb: cell count
	return:
		flash physical address, and it is including crc field;
 */
int dm_fwrite(uint32_t addr, void *ptr, size_t size, size_t nmemb)
{
    int original_len = size * nmemb;
	int handle_count, hdl_in_part_cnt;
	int cell_cnt, remnant_cnt;
	char *content;

	log_i("dm_fwrite:0x%x,:0x%x, %d", addr, ptr, original_len);

	/* handle the buffer, including efuse encryption and instrution crc*/
	handle_count = (remnant_len + original_len) / INS_NO_CRC_CHUNK * INS_NO_CRC_CHUNK;
	remnant_cnt = (remnant_len + original_len) - handle_count;
	hdl_in_part_cnt = handle_count - remnant_len;
	
	cell_cnt = dm_efuse_enc_buf_convert(remnant, remnant_len, ptr, pre_encrypt_buf, hdl_in_part_cnt);
    encrypt((u32 *)pre_encrypt_buf, (u8 *)flash_buf, cell_cnt, addr);
    calc_crc((u32 *)flash_buf, cell_cnt);

	/*write flash, and the data has crc field*/
    flash_write_data((unsigned char *)flash_buf, addr, handle_count * INSTRUCTION_CRC_CHUNK / INS_NO_CRC_CHUNK);

	/*record the unalignment data for next writing*/
	content = (char *)ptr;
	dm_stay_unalignment_remant(&content[hdl_in_part_cnt], remnant_cnt);
	
    return handle_count * INSTRUCTION_CRC_CHUNK / INS_NO_CRC_CHUNK;
}

int dm_fwrite_tail(uint32_t addr)
{
	int padding_len, i;
	uint8_t *padding_ptr;
	
	log_i("dm_fwrite_tail:0x%x,:0x%x, %d", addr, remnant, remnant_len);

	padding_len = INS_NO_CRC_CHUNK - remnant_len;
	padding_ptr = &remnant[remnant_len];
	for(i = 0; i < padding_len; i ++)
	{
		padding_ptr[i] = 0xff;
	}
	
	remnant_len = INS_NO_CRC_CHUNK;
	dm_fwrite(addr, 0, 0, 0);

	return padding_len;
}	
void *mem_cpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;
 
	while (count--)
		*tmp++ = *s++;
	return dest;
}
int data_move_handler(void)
{
        unsigned have;
        z_stream strm;
        uint32_t dest_addr, src_addr;
        int ret, src_offset = 0;
        int dl_partition_len = 0;
        int dl_valid_data_len = 0;
        int dl_total_len = 0;
        int rd_len, req_len, wr_ret;
        Bytef *zlib_decompress_ptr;
        struct ota_rbl_hdr  rbl_hdr;
        fal_partition_t dest_part = NULL;
        fal_partition_t src_part  = NULL;
        int handle_count;
        int padding_len;
        char  *padding_ptr = NULL;
        char  *contentx = NULL;
        int i = 0;

        clr_flash_protect(); // 4
        src_part  = fal_partition_find(RT_BK_DL_PART_NAME);
        bk_printf("fal_partition_find over: ");    
        if(data_move_start(src_part))
        {
                return RET_DM_NO_OTA_DATA;
        }
        dl_partition_len = src_part->len;
        //read firmware head from download partition 96 bytes
        fal_get_fw_hdr(RT_BK_DL_PART_NAME, &rbl_hdr);
        dest_part = fal_partition_find(rbl_hdr.name);
        dl_valid_data_len = rbl_hdr.size_package;	
        //dm_erase_dest_partition(APP_SEC_BASE, (APP_SEC_LEN + (SM_SECTOR - 1)) / SM_SECTOR * SM_SECTOR);
        dm_erase_dest_partition(dest_part->offset, (dest_part->len + (SM_SECTOR - 1)) / SM_SECTOR * SM_SECTOR);
        dest_addr = dest_part->offset;
        src_addr = src_part->offset+ 0x60; //0x60:rbl head size
        bk_printf("src_address: ");
        bk_print_hex(src_addr);
        bk_printf("\r\n:");
        bk_printf("dest_address: ");
        bk_print_hex(dest_addr);
        bk_printf("\r\n:");     
        /* allocate inflate state */
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        log_i("inflateInit:0x%x", &strm);
        ret = inflateInit(&strm);
        if (ret != Z_OK)
            return ret;
        while((dl_valid_data_len > src_offset) && (ret != Z_STREAM_END))
        {
                req_len = MIN(DM_CHUNK, (dl_valid_data_len - src_offset));
                rd_len = dm_rd_src_partition(src_addr, (char *)in_buf, req_len);
                ASSERT(rd_len == req_len);
                ASSERT(0 == (rd_len & (32 - 1)));
                src_addr += rd_len;
                src_offset += rd_len;
                bk_printf("\r\n");
                //log_i("src_addr:0x%x:0x%x:0x%x", src_addr, dl_total_len, src_offset);
                bk_printf("src_addr, src_offset ");
                bk_print_hex(src_addr);
                bk_print_hex(src_offset);
                bk_printf("\r\n:");
                //aes_decrypt_handler((char *)in_buf, (char *)out_buf, rd_len);
                strm.avail_in = rd_len;
                if (strm.avail_in == 0)
                        break;
                strm.next_in = (Bytef *)in_buf;
                zlib_decompress_ptr = (Bytef *)out_buf;
            /* run inflate() on input until output buffer not full */
                do
                {
                        strm.avail_out = ZLIB_CHUNK;
                        strm.next_out = zlib_decompress_ptr;
                        //log_i("inflate in:0x%x 0x%x out:0x%x 0x%x", strm.avail_in, strm.next_in, strm.avail_out, strm.next_out);
                        bk_printf("strm.avail_in, strm.next_in, strm.avail_out, strm.next_out");
                        bk_print_hex(strm.avail_in);
                        bk_print_hex(strm.next_in);
                        bk_print_hex(strm.avail_out);
                        bk_print_hex(strm.next_out);
                        bk_printf("\r\n:");
#if CFG_BEKEN_OTA
                if(0xaa > strm.avail_in)
                {
                        dump_hex((uint8_t *)strm.next_in, strm.avail_in);
                }
#endif
                ret = inflate(&strm, Z_NO_FLUSH);
                ASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */
                bk_printf("inflate ret");
                bk_print_hex(ret);
                bk_printf("\r\n:");
                switch (ret)
                {
                        case Z_NEED_DICT:
                                ret = Z_DATA_ERROR;     /* and fall through */
                        case Z_DATA_ERROR:
                        case Z_MEM_ERROR:
                                //log_i("inflateEnd:0x%x", ret);
                                bk_printf("inflateEnd ret");
                                bk_print_hex(ret);
                                bk_printf("\r\n:");
                                (void)inflateEnd(&strm);
                                return ret;
                }
                have = ZLIB_CHUNK - strm.avail_out;
                bk_printf("have:");
                bk_print_hex(have);
                bk_printf("\r\n:");
                REG_FLASH_CONFIG  |= FLASH_CONFIG_CPU_WRITE_ENABLE_MASK; //cpu write flash enable       
                contentx = (char *)&out_buf[0];    
                padding_len = INS_NO_CRC_CHUNK - remnant_len;
#if 1//cpu write data start
                if(remnant_len != 0)
                {
                        mem_cpy(&remnantx[remnant_len],&contentx[0],padding_len);
                        mem_cpy((USER_APP_ENTRY+src_offset1), &remnantx[0], INS_NO_CRC_CHUNK);        
                        src_offset1 += 32;
                        handle_count = have -(padding_len)- ((have - (padding_len))%32);
                        mem_cpy((USER_APP_ENTRY+src_offset1), &contentx[(padding_len)], handle_count);            
                        src_offset1 += handle_count;
                        mem_cpy(&remnantx[0],&contentx[(handle_count+padding_len)],(have-handle_count-padding_len));
                        remnant_len = have-handle_count-padding_len;
                }
                else
                {
                        if(have%32 ==0)
                        {
                                mem_cpy((USER_APP_ENTRY+src_offset1), &contentx[0], have);
                                src_offset1 += have;
                        }
                        else
                        {
                                handle_count = have - (have)%32;
                                mem_cpy((USER_APP_ENTRY+src_offset1), &contentx[0], handle_count);
                                src_offset1 += handle_count;                   
                                mem_cpy(&remnantx[0],&contentx[handle_count],have-handle_count);
                                remnant_len = have-handle_count;                   
                        }
                }
#endif            		
                }while (strm.avail_out == 0);
        //__asm volatile ("j .");      
                /* done when inflate() says it's done */
        }
        if(remnant_len != 0)
        {	
                padding_len = INS_NO_CRC_CHUNK - remnant_len;
                padding_ptr = &remnantx[remnant_len];
                for(i = 0; i < padding_len; i ++)
                {
                        padding_ptr[i] = 0xff;
                }
                mem_cpy(&remnantx[remnant_len],&padding_ptr[0],padding_len);
                mem_cpy((USER_APP_ENTRY+src_offset1), &remnantx[0], INS_NO_CRC_CHUNK);
                remnant_len = 0;
        }//cpu write data end
        REG_READ(0x0);        
        REG_FLASH_CONFIG  &= (~(FLASH_CONFIG_CPU_WRITE_ENABLE_MASK)); //cpu write flash disable
        /* clean up and return */
        (void)inflateEnd(&strm);
        //erase dl partition
        src_part  = fal_partition_find(RT_BK_DL_PART_NAME);
        data_move_end(src_part->offset);
        return ret == Z_STREAM_END ? RET_DM_SUCCESS : RET_DM_DATA_ERROR;
}
// eof

