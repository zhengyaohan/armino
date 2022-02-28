#define OTA_PART_PARAMETER_1  0x10000
#define FLASH_SIZE            0x200000  
#define FLASH_SECTOR_SIZE     0x1000
#define OTA_UPG_FLAG          0xA55A
#define OTA_PINGPONG_FLAG     0xB55B

#define XZ_BUF_SIZE 2048
#define OTA_XZ_PARAM_FAIL  -1
#define OTA_XZ_CRC_FAIL    -2
#define OTA_XZ_UNCOMP_FAIL  -3
#define OTA_XZ_VERIFY_FAIL  -4

typedef struct
{
    u32 dst_adr;
    u32 src_adr;
    u32 size;
    u16 crc;
    u16 upg_flag;  /*upgrade flag  0xA55A->xz compress bin*/
} ota_param_t;

int ota_update(void);   /*OTA main */

int ota_flash_erase(u32 addr); /* erase sector */
int ota_flash_read(u32 addr, uint8_t *buf, u32 len);
int ota_flash_write(u32 addr, uint8_t *buf, u32 len);


