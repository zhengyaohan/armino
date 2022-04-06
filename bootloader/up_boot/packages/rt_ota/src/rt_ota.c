/*
 * File      : rt_ota.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#include <rt_ota.h>
#include <string.h>
#include <stdlib.h>

#ifdef RT_OTA_CRYPT_ALGO_USING_AES256
#include <tiny_aes.h>
#endif

#ifdef RT_OTA_CMPRS_ALGO_USING_QUICKLZ
#include <quicklz.h>
#endif

#ifdef RT_OTA_USING_DL_RESUME
#include <easyflash.h>
#endif

/* rbl file decrypt or cmp block size */
#define DECRYPT_DCMPRS_BUFFER_SIZE     4096
//TODO 具体参数待实测优化

static uint8_t init_ok = 0;
/* partitions total number */
static size_t parts_num = 0;
/* rbl file header length */
static const size_t rbl_hdr_len = sizeof(struct rt_ota_rbl_hdr);

/**
 * OTA initialization
 *
 * @return -1: partition table not found
 *         -2: download partition not found
 *        >=0: initialize success
 */
int rt_ota_init(void)
{
    int result = 0;

    if (init_ok)
    {
        return 0;
    }

    rt_ota_flash_device_init();

    parts_num = rt_ota_partition_init();

    if (parts_num == 0)
    {
        log_e("Initialize failed! Don't found the partition table.");
        result = -1;
        goto __exit;
    }

    if (!rt_ota_partition_find(RT_OTA_DL_PART_NAME))
    {
        log_e("Initialize failed! The download partition(%s) not found.", RT_OTA_DL_PART_NAME);
        result = -2;
        goto __exit;
    }

__exit:

    if (!result){
        log_i("OTA (V%s) success.", RT_OTA_SW_VERSION);
    } else {
        log_e("RT-Thread OTA package(V%s) initialize failed(%d).", RT_OTA_SW_VERSION, result);
    }

    init_ok = 1;

    return result;
}

/**
 * get firmware header on this partition
 *
 * @param part_name partition name
 * @param hdr firmware header
 *
 * @return -1: get firmware header has an error, >=0: success
 */
int rt_ota_get_fw_hdr(const char *part_name, struct rt_ota_rbl_hdr *hdr)
{
    const struct rt_ota_partition *part = rt_ota_partition_find(part_name);
    uint32_t crc32 = 0;

    assert(init_ok);
    assert(part_name);
    assert(hdr);

    if (!part)
    {
        log_e("Get firmware header error. The partition %s was not found", part_name);
        return -1;
    }

    if (!strcmp(part_name, RT_OTA_DL_PART_NAME))
    {
        /* firmware header is on OTA download partition top */
        rt_ota_partition_read(part, 0, (uint8_t *) hdr, rbl_hdr_len);
    }
    else
    {
        /* firmware header is on other partition bottom */
        rt_ota_partition_read(part, part->len - rbl_hdr_len, (uint8_t *) hdr, rbl_hdr_len);
    }

    if (hdr->info_crc32 != (crc32 = rt_ota_calc_crc32(0, hdr, rbl_hdr_len - sizeof(uint32_t))))
    {
        log_e("Get firmware header occur CRC32(calc.crc: %08lx != hdr.info_crc32: %08lx) error on '%s' partition!", crc32,
                hdr->info_crc32, part_name);
        return -1;
    }

    return 0;
}

/**
 * get downloaded firmware address in '.rbl' file on OTA download partition
 *
 * @return firmware relative address for OTA download partition
 */
uint32_t rt_ota_get_save_rbl_body_addr(void)
{
    assert(init_ok);

    return rbl_hdr_len;
}

/**
 * save firmware header on this partition
 *
 * @param part_name partition name
 * @param hdr firmware header
 *
 * @note MUST erase partition before save firmware header
 *
 * @return -1: save failed, >=0: success
 */
int rt_ota_save_fw_hdr(const char *part_name, struct rt_ota_rbl_hdr *hdr)
{
    const struct rt_ota_partition *part = rt_ota_partition_find(part_name);

    assert(init_ok);
    assert(part_name);
    assert(strcmp(part_name, RT_OTA_DL_PART_NAME));
    assert(hdr);

    if (!part)
    {
        log_e("Save failed. The partition %s was not found.", part_name);
        return -1;
    }

    /* save firmware header on partition bottom */
    return rt_ota_partition_write(part, part->len - rbl_hdr_len, (uint8_t *) hdr, rbl_hdr_len);
}

/**
 * verify firmware hash code on this partition
 *
 * @param part partition @note this partition is not 'OTA download' partition
 * @param hdr firmware header
 *
 * @return -1: failed, >=0: success
 */
static int part_fw_hash_verify(const struct rt_ota_partition *part, const struct rt_ota_rbl_hdr *hdr)
{
    uint32_t fw_start_addr, fw_end_addr, hash = RT_OTA_HASH_FNV_SEED, i;
    uint8_t buf[32], remain_size;

    assert(strcmp(part->name, RT_OTA_DL_PART_NAME));

    fw_start_addr = 0;
    fw_end_addr = fw_start_addr + hdr->size_raw;
    /* calculate hash */
    for (i = fw_start_addr; i <= fw_end_addr - sizeof(buf); i += sizeof(buf))
    {
        rt_ota_partition_read(part, i, buf, sizeof(buf));
        hash = rt_ota_calc_hash(hash, buf, sizeof(buf));
    }
    /* align process */
    if (i != fw_end_addr - sizeof(buf))
    {
        remain_size = fw_end_addr - i;
        rt_ota_partition_read(part, i, buf, remain_size);
        hash = rt_ota_calc_hash(hash, buf, remain_size);
    }

    if (hash != hdr->hash)
    {
        log_e("Verify firmware hash(calc.hash: %08lx != hdr.hash: %08lx) failed on partition '%s'.", hash, hdr->hash,
                part->name);
        return -1;
    }

    return 0;
}

/**
 * verify firmware on this partition
 *
 * @param part partition
 *
 * @return -1: failed, >=0: success
 */
int rt_ota_part_fw_verify(const struct rt_ota_partition *part)
{
    struct rt_ota_rbl_hdr hdr;
    uint32_t fw_start_addr, fw_end_addr, i, crc32 = 0;
    uint8_t buf[32], remain_size;

    assert(init_ok);
    assert(part);

    if (rt_ota_get_fw_hdr(part->name, &hdr) < 0)
    {
        return -1;
    }

    if (!strcmp(part->name, RT_OTA_DL_PART_NAME))
    {
        /* on OTA download partition */
        assert(hdr.size_package >= sizeof(buf));

        fw_start_addr = rt_ota_get_save_rbl_body_addr();
        fw_end_addr = fw_start_addr + hdr.size_package;
        /* calculate CRC32 */
        for (i = fw_start_addr; i <= fw_end_addr - sizeof(buf); i += sizeof(buf))
        {
            rt_ota_partition_read(part, i, buf, sizeof(buf));
            crc32 = rt_ota_calc_crc32(crc32, buf, sizeof(buf));
        }
        /* align process */
        if (i != fw_end_addr - sizeof(buf))
        {
            remain_size = fw_end_addr - i;
            rt_ota_partition_read(part, i, buf, remain_size);
            crc32 = rt_ota_calc_crc32(crc32, buf, remain_size);
        }

        if (crc32 != hdr.crc32)
        {
            log_e("Verify firmware CRC32(calc.crc: %08lx != hdr.crc: %08lx) failed on partition '%s'.", crc32, hdr.crc32, part->name);
            return -1;
        }
    }
    else if (part_fw_hash_verify(part, &hdr) < 0)
    {
        return -1;
    }

    log_i("Verify '%s' partition(fw ver: %s, timestamp: %ld) success.", part->name, hdr.version, hdr.timestamp);

    return 0;
}

/**
 * check need to upgrade
 *
 * @note please run `rt_ota_part_fw_verify` before upgrade
 *
 * @return 1: need upgrade, 0: don't need upgrade
 */
int rt_ota_check_upgrade(void)
{
    struct rt_ota_rbl_hdr dl_hdr, dest_hdr;

    assert(init_ok);

    if (rt_ota_get_fw_hdr(RT_OTA_DL_PART_NAME, &dl_hdr) < 0)
    {
        log_e("Get OTA download partition firmware header failed!");
        return 0;
    }

    if (rt_ota_get_fw_hdr(dl_hdr.name, &dest_hdr) < 0)
    {
        log_e("Get '%s' partition firmware header failed! This partition will be forced to upgrade.", dl_hdr.name);
        return 1;
    }
    /* compare firmware header */
    if (!memcmp(&dl_hdr, &dest_hdr, sizeof(struct rt_ota_rbl_hdr)))
    {
        return 0;
    }

    return 1;
}

static void print_progress(size_t cur_size, size_t total_size)
{
    static unsigned char progress_sign[100 + 1];
    uint8_t i, per = cur_size * 100 / total_size;

    if (per > 100)
    {
        per = 100;
    }

    for (i = 0; i < 100; i++)
    {
        if (i < per)
        {
            progress_sign[i] = '=';
        }
        else if (per == i)
        {
            progress_sign[i] = '>';
        }
        else
        {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';

    log_d("\033[2A");
    log_i("OTA Write: [%s] %d%%", progress_sign, per);
}

/**
 * copy downloader data to the specified partition
 *
 * @param part partition
 * @param hdr firmware header
 *
 * @return -1: failed, 0: success
 */
static int copy_fw_from_dl_part(const struct rt_ota_partition *part, struct rt_ota_rbl_hdr *hdr)
{
    const struct rt_ota_partition *dl_part;
    uint8_t *buffer_in = NULL, *buffer_out = NULL;
    size_t len, i;
    int ret = 0;

    if ((dl_part = rt_ota_partition_find(RT_OTA_DL_PART_NAME)) == NULL)
    {
        log_e("Copy firmware from downloader partition failed! Partition(%s) not found!", RT_OTA_DL_PART_NAME);
        ret = -1;
        goto _exit;
    }

    buffer_in = (uint8_t *) RT_OTA_MALLOC(DECRYPT_DCMPRS_BUFFER_SIZE);
    buffer_out = (uint8_t *) RT_OTA_MALLOC(DECRYPT_DCMPRS_BUFFER_SIZE);
    if (!buffer_in || !buffer_out)
    {
        log_e("Copy firmware from downloader partition failed! No memory for buffer_in or buffer_out!");
        ret = -1;
        goto _exit;
    }
    memset(buffer_in, 0x00, DECRYPT_DCMPRS_BUFFER_SIZE);
    memset(buffer_out, 0x00, DECRYPT_DCMPRS_BUFFER_SIZE);

#ifdef RT_OTA_CRYPT_ALGO_USING_AES256
    tiny_aes_context ctx;
    uint8_t iv[16 + 1];
    uint8_t private_key[32 + 1];

    extern void rt_ota_get_iv_key(uint8_t * iv_buf, uint8_t * key_buf);
    rt_ota_get_iv_key(iv, private_key);

    iv[sizeof(iv) - 1] = '\0';
    private_key[sizeof(private_key) - 1] = '\0';
    tiny_aes_setkey_dec(&ctx, (uint8_t *) private_key, 256);
    log_d("AES KEY:%s IV:%s", private_key, iv);
#endif

    for (i = 0; i < hdr->size_package; i += DECRYPT_DCMPRS_BUFFER_SIZE)
    {
        if ((hdr->size_package - i) < DECRYPT_DCMPRS_BUFFER_SIZE)
        {
            len = hdr->size_package - i;
        }
        else
        {
            len = DECRYPT_DCMPRS_BUFFER_SIZE;
        }

        /* read the partition data from the header's length address */
        if (rt_ota_partition_read(dl_part, i + sizeof(struct rt_ota_rbl_hdr), buffer_in, len) < 0)
        {
            log_e("Copy firmware from downloader partition failed! OTA partition(%s) read error!", dl_part->name);

            ret = -1;
            break;
        }

        if (hdr->algo == RT_OTA_CRYPT_ALGO_AES256)
        {
#ifdef RT_OTA_CRYPT_ALGO_USING_AES256
            tiny_aes_crypt_cbc(&ctx, AES_DECRYPT, len, iv, buffer_in, buffer_out);
#else
            log_e("Not supported AES256 firmware, please check you configuration!");
            ret = -1;
            goto _exit;
#endif
        }
        else
        {
            memcpy(buffer_out, (const uint8_t *) buffer_in, len);
        }

        if (rt_ota_partition_write(part, i, (const uint8_t *) buffer_out, len) < 0)
        {
            log_e("Copy firmware from downloader partition failed! OTA partition(%s) write error!", part->name);
            ret = -1;
            break;
        }
        print_progress(i, hdr->size_package);
    }
    print_progress(i, hdr->size_package);

_exit:
    if (buffer_in)
    {
        RT_OTA_FREE(buffer_in);
    }

    if (buffer_out)
    {
        RT_OTA_FREE(buffer_out);
    }

    return ret;
}

#if defined(RT_OTA_CRYPT_ALGO_USING_AES256) && defined(RT_OTA_CMPRS_ALGO_USING_QUICKLZ)

#define CRYPT_ALIGN_SIZE               16                  /* AES decryption block size requires 16-byte alignment */
#define CMPRS_HEADER_SIZE              4                   /* The header before cmpress block is used to store the block size that needs to be decompressed */
#define CMPRS_PADDING_SIZE             QLZ_BUFFER_PADDING  /* Padding is used to provide a buffer for decompressing data */
#define CMPRS_BUFFER_SIZE              (CMPRS_HEADER_SIZE + DECRYPT_DCMPRS_BUFFER_SIZE +CMPRS_PADDING_SIZE + CRYPT_ALIGN_SIZE)

/**
 * Decrypt & decompress & copy downloader data to the specified partition
 *
 * @param part partition
 * @param hdr firmware header
 *
 * @return -1: failed, 0: success
 */
static int decrypt_dcmprs_fw_from_dl_part(const struct rt_ota_partition *part, struct rt_ota_rbl_hdr *hdr)
{
    qlz_state_decompress *dcmprs_state = NULL;
    uint8_t *buffer_in = NULL, *buffer_out = NULL, *buffer_cmprs = NULL;
    const struct rt_ota_partition *dl_part;
    int ret = 0;

    if ((dl_part = rt_ota_partition_find(RT_OTA_DL_PART_NAME)) == NULL)
    {
        log_e("decrypt & decompress firmware from download partition failed! Partition(%s) not found!", RT_OTA_DL_PART_NAME);
        ret = -1;
        goto _exit;
    }

    buffer_in = (uint8_t *) RT_OTA_MALLOC(CMPRS_BUFFER_SIZE);
    buffer_out = (uint8_t *) RT_OTA_MALLOC(CMPRS_BUFFER_SIZE);
    if (!buffer_in || !buffer_out)
    {
        log_e("decrypt & decompress firmware from download partition failed! No memory for buffer_in or buffer_out!");
        ret = -1;
        goto _exit;
    }
    memset(buffer_in, 0x00, CMPRS_BUFFER_SIZE);
    memset(buffer_out, 0x00, CMPRS_BUFFER_SIZE);

#ifdef RT_OTA_CRYPT_ALGO_USING_AES256
    tiny_aes_context ctx;
    uint8_t iv[16 + 1];
    uint8_t private_key[32 + 1];

    extern void rt_ota_get_iv_key(uint8_t * iv_buf, uint8_t * key_buf);
    rt_ota_get_iv_key(iv, private_key);

    iv[sizeof(iv) - 1] = '\0';
    private_key[sizeof(private_key) - 1] = '\0';
    tiny_aes_setkey_dec(&ctx, (uint8_t *) private_key, 256);
    log_d("AES KEY:%s IV:%s", private_key, iv);
#endif

    dcmprs_state = (qlz_state_decompress *) RT_OTA_MALLOC(sizeof(qlz_state_decompress));
    if (!dcmprs_state)
    {
        log_e("decrypt & decompress firmware from downloader partition failed! No memory for qlz_state_decompress struct !");
        ret = -1;
        goto _exit;
    }
    memset(dcmprs_state, 0x00, sizeof(qlz_state_decompress));

    buffer_cmprs =  (uint8_t *) RT_OTA_MALLOC(CMPRS_BUFFER_SIZE);
    if(!buffer_cmprs)
    {
        log_e("decrypt & decompress firmware from downloader partition failed! No memory for buffer_cmprs!");
        ret = -1;
        goto _exit;
    }
    memset(buffer_cmprs, 0x00, CMPRS_BUFFER_SIZE);

    /* compression block header get the size of block */
    uint8_t cmprs_block_header[4] = { 0 };

    /* the size of the decrypt block in every cycle*/
    size_t decrypt_block_size = 0;

    /* The size of the data that has been decompress, Compared with the "hdr->size_package" to judge the cycle execute or not*/
    size_t already_dcmprs_size = 0;

    /* crmps_buffer_size : the size of the currently available compressed data that read from the flash, length range 4096 - 4109.
     * crmps_buffer_remain_size : remained the length of the compressed data after the current decompression .
     * */
    size_t crmps_buffer_size = 0, crmps_buffer_remain_size = 0;

    size_t crmps_block_size = 0, dcrmps_block_size = 0;
    size_t flash_already_read_size = 0, flash_already_write_size = 0;

    do
    {
        if (already_dcmprs_size == 0)
        {
            crmps_buffer_size = CMPRS_BUFFER_SIZE - (CMPRS_BUFFER_SIZE % CRYPT_ALIGN_SIZE);
            decrypt_block_size = crmps_buffer_size;
        }
        else
        {
            /* The size of the currently need to decrypt that is the last compress size of 16 bytes aligned(AES256) */
            if (hdr->algo == (RT_OTA_CRYPT_ALGO_AES256 | RT_OTA_CMPRS_ALGO_QUICKLZ))
            {
                decrypt_block_size = CMPRS_BUFFER_SIZE - crmps_buffer_remain_size
                        - (CMPRS_BUFFER_SIZE - crmps_buffer_remain_size) % CRYPT_ALIGN_SIZE;
            }
            else
            {
                decrypt_block_size = crmps_block_size + CMPRS_HEADER_SIZE;
            }

            crmps_buffer_size = crmps_buffer_remain_size + decrypt_block_size;
        }

        if ((hdr->size_package - flash_already_read_size) <= decrypt_block_size)
        {
            decrypt_block_size = hdr->size_package - flash_already_read_size;
        }

        if (decrypt_block_size > 0)
        {
            /* read the partition data from the header's length address */
            if (rt_ota_partition_read(dl_part, sizeof(struct rt_ota_rbl_hdr) + flash_already_read_size , buffer_in,
                    decrypt_block_size) < 0)
            {
                log_e("decrypt & decompress firmware from downloader partition failed! OTA partition(%s) read error!", dl_part->name);
                ret = -1;
                break;
            }
            flash_already_read_size += decrypt_block_size;

            if (hdr->algo == (RT_OTA_CRYPT_ALGO_AES256 | RT_OTA_CMPRS_ALGO_QUICKLZ))
            {
#ifdef RT_OTA_CRYPT_ALGO_USING_AES256
                tiny_aes_crypt_cbc(&ctx, AES_DECRYPT, decrypt_block_size, iv, buffer_in, buffer_cmprs + crmps_buffer_remain_size);
#else
                log_e("Not supported AES256 firmware, please check you configuration!");
                ret = -1;
                goto _exit;
#endif
            }
            else
            {
                memcpy(buffer_cmprs + crmps_buffer_remain_size, buffer_in, decrypt_block_size);
            }
        }

        /* read the padding data , end decompression and decryption process */
        memcpy(cmprs_block_header, buffer_cmprs, CMPRS_HEADER_SIZE);
        if (cmprs_block_header[0])
        {
            print_progress(hdr->size_package, hdr->size_package);
            log_d("decrypt & decompress firmware from downloader partition success!");
            break;
        }

        /* get the compression block size by the compression block header(4 byte) */
        crmps_block_size = cmprs_block_header[0] * (1 << 24) + cmprs_block_header[1] * (1 << 16)
                + cmprs_block_header[2] * (1 << 8) + cmprs_block_header[3];
        assert(crmps_block_size <= crmps_buffer_size);

        memset(buffer_in, 0x00, CMPRS_BUFFER_SIZE);
        memcpy(buffer_in, buffer_cmprs + CMPRS_HEADER_SIZE, crmps_block_size);

        /* get the decompression block size */
        dcrmps_block_size = qlz_decompress((const char *) buffer_in, buffer_out, dcmprs_state);

        if (rt_ota_partition_write(part, flash_already_write_size, (const uint8_t *) buffer_out, dcrmps_block_size) < 0)
        {
            log_e("decrypt & decompress firmware from downloader partition failed! OTA partition(%s) write error!", part->name);
            ret = -1;
            break;
        }
        flash_already_write_size += dcrmps_block_size;

        /* copy the remain compression buffer size to the buffer header */
        crmps_buffer_remain_size = crmps_buffer_size - crmps_block_size - CMPRS_HEADER_SIZE;
        memset(buffer_in, 0x00, CMPRS_BUFFER_SIZE);
        memcpy(buffer_in, buffer_cmprs + crmps_block_size + CMPRS_HEADER_SIZE, crmps_buffer_remain_size);
        memset(buffer_cmprs, 0x00, CMPRS_BUFFER_SIZE);
        memcpy(buffer_cmprs, buffer_in, crmps_buffer_remain_size);

        already_dcmprs_size += crmps_block_size + CMPRS_HEADER_SIZE;

        print_progress(already_dcmprs_size, hdr->size_package);
    } while (already_dcmprs_size < hdr->size_package);

    if(already_dcmprs_size == hdr->size_package)
    {
        print_progress(already_dcmprs_size, hdr->size_package);
        log_d("decrypt & decompress firmware from downloader partition success!");
    }

_exit:
    if (buffer_in)
    {
        RT_OTA_FREE(buffer_in);
    }

    if (buffer_out)
    {
        RT_OTA_FREE(buffer_out);
    }

    if (buffer_cmprs)
    {
        RT_OTA_FREE(buffer_cmprs);
    }

    return ret;
}
#endif

/**
 * upgrade firmware from OTA download partition
 *
 * @note please run `rt_ota_check_upgrade` before upgrade
 *
 * @return -1: upgrade failed, 0: upgrade success
 */
int rt_ota_upgrade(void)
{
    struct rt_ota_rbl_hdr dl_hdr, old_hdr;
    const struct rt_ota_partition *dest_part;

    assert(init_ok);

    if (rt_ota_get_fw_hdr(RT_OTA_DL_PART_NAME, &dl_hdr) < 0)
    {
        log_e("Get OTA download partition firmware header failed!");
        return -1;
    }

    if ((dest_part = rt_ota_partition_find(dl_hdr.name)) == NULL)
    {
        log_e("Partition(%s) not found!", dl_hdr.name);
        return -1;
    }


    if (rt_ota_get_fw_hdr(dl_hdr.name, &old_hdr) >= 0)
    {
        log_i("OTA firmware(%s) upgrade(%s->%s) startup.", dl_hdr.name, old_hdr.version, dl_hdr.version);
        log_d("original version: %s, timestamp: %ld", old_hdr.version, old_hdr.timestamp);
    }
    else
    {
        log_i("OTA firmware(%s) upgrade startup.", dl_hdr.name);
    }
    log_d("new firmware version: %s, timestamp: %ld", dl_hdr.version, dl_hdr.timestamp);

    flash_set_protect(0);
    
    if (dl_hdr.algo == RT_OTA_CRYPT_ALGO_NONE)
    {
        log_i("The partition '%s' is erasing.", dest_part->name);
        /* erase destination partition all data */
        rt_ota_partition_erase_all(dest_part);
        /* Loop block copy data to memory, and write to the specified partition of rbl header */
        if (copy_fw_from_dl_part(dest_part, &dl_hdr) < 0)
        {
            log_e("OTA upgrade failed! Downloader data copy to partition(%s) error!", dest_part->name);
            goto RE_ERR;
        }
    }
#if defined(RT_OTA_CRYPT_ALGO_USING_AES256) && defined(RT_OTA_CMPRS_ALGO_USING_QUICKLZ)
    else if (dl_hdr.algo == (RT_OTA_CRYPT_ALGO_AES256 | RT_OTA_CMPRS_ALGO_QUICKLZ))
    {
        /* erase destination partition all data */
        rt_ota_partition_erase_all(dest_part);
        log_i("The partition '%s' erase success.", dest_part->name);
        /* Loop block decrypt & unzip data to memory, and write to the specified partition of rbl header */
        if (decrypt_dcmprs_fw_from_dl_part(dest_part, &dl_hdr) < 0)
        {
            log_e("OTA upgrade failed! Downloader data decrypt & compress & copy to partition(%s) error!", dest_part->name);
            goto RE_ERR;
        }
    }
#endif
#ifdef RT_OTA_CMPRS_ALGO_USING_QUICKLZ
    else if (dl_hdr.algo == RT_OTA_CMPRS_ALGO_QUICKLZ)
    {
        /* erase destination partition all data */
        rt_ota_partition_erase_all(dest_part);
        log_i("The partition '%s' erase success.", dest_part->name);
        /* Loop block decrypt data to memory, and write to the specified partition of rbl header */
        if (decrypt_dcmprs_fw_from_dl_part(dest_part, &dl_hdr) < 0)
        {
            log_e("OTA upgrade failed! Downloader data compress & copy to partition(%s) error!", dest_part->name);
            goto RE_ERR;
        }
    }
#endif
#ifdef RT_OTA_CRYPT_ALGO_USING_AES256
    else if (dl_hdr.algo == RT_OTA_CRYPT_ALGO_AES256)
    {
        /* erase destination partition all data */
        rt_ota_partition_erase_all(dest_part);
        log_i("The partition '%s' erase success.", dest_part->name);
        /* Loop block decrypt data to memory, and write to the specified partition of rbl header */
        if (copy_fw_from_dl_part(dest_part, &dl_hdr) < 0)
        {
            log_e("OTA upgrade failed! Downloader data decrypt & copy to partition(%s) error!", dest_part->name);
            goto RE_ERR;
        }
    }
#endif
    else
    {
        log_e("OTA upgrade failed! Downloader header algo(0x%x) is not supported!", dl_hdr.algo);
        goto RE_ERR;
    }

    /* verify destination partition firmware hash code */
    if (part_fw_hash_verify(dest_part, &dl_hdr) < 0)
    {
        goto RE_ERR;
    }

    flash_set_protect(1);
    return rt_ota_save_fw_hdr(dl_hdr.name, &dl_hdr);

RE_ERR:
    flash_set_protect(1);
    return -1;
}
