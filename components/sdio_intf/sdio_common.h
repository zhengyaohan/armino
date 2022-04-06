/*
 * wpa_supplicant/hostapd / common helper functions, etc.
 * Copyright (c) 2002-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef COMMON_H
#define COMMON_H

#include <common/bk_include.h>


#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x "

/*
 * Compact form for string representation of MAC address
 * To be used, e.g., for constructing dbus paths for P2P Devices
 */
#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"
#endif

#ifndef BIT
#define BIT(x) (1U << (x))
#endif

int hwaddr_aton(const char *txt, u8 *addr);
int hwaddr_masked_aton(const char *txt, u8 *addr, u8 *mask, u8 maskable);
int hwaddr_compact_aton(const char *txt, u8 *addr);
int hwaddr_aton2(const char *txt, u8 *addr);
int hex2byte(const char *hex);
int hexstr2bin(const char *hex, u8 *buf, size_t len);
void inc_byte_array(u8 *counter, size_t len);
int hwaddr_mask_txt(char *buf, size_t len, const u8 *addr, const u8 *mask);
void printf_encode(char *txt, size_t maxlen, const u8 *data, size_t len);
size_t printf_decode(u8 *buf, size_t maxlen, const char *str);
const char *wpa_ssid_txt(const u8 *ssid, size_t ssid_len);
int is_hex(const u8 *data, size_t len);
size_t merge_byte_arrays(u8 *res, size_t res_len,
						 const u8 *src1, size_t src1_len,
						 const u8 *src2, size_t src2_len);
char *dup_binstr(const void *src, size_t len);

static inline int is_zero_ether_addr(const u8 *a)
{
	return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

static inline int is_broadcast_ether_addr(const u8 *a)
{
	return (a[0] & a[1] & a[2] & a[3] & a[4] & a[5]) == 0xff;
}

static inline int is_multicast_ether_addr(const u8 *a)
{
	return a[0] & 0x01;
}

#define broadcast_ether_addr (const u8 *) "\xff\xff\xff\xff\xff\xff"

void str_clear_free(char *str);
void bin_clear_free(void *bin, size_t len);

int random_mac_addr(u8 *addr);
int random_mac_addr_keep_oui(u8 *addr);

const char *cstr_token(const char *str, const char *delim, const char **last);
char *str_token(char *str, const char *delim, char **context);
size_t utf8_escape(const char *inp, size_t in_size,
				   char *outp, size_t out_size);
size_t utf8_unescape(const char *inp, size_t in_size,
					 char *outp, size_t out_size);
int is_ctrl_char(char c);


#endif /* COMMON_H */
