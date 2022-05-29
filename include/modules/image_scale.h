#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/* @brief Overview about lcds demo
 *
 */

/**
 * @brief lcd Demo mocro
 * @defgroup lcd demo group
 * @{
 */

/**
 * @brief according target image scale crop src image, and compress target image
 *	for exampe: src image is 1280*720P and target image is 320*480, you should crop the maximul picture to scale compress target picture.
 *	so this algorithns can crop w*h is 480*720 to scale 320*480p.
 */
int image_scale_crop_compress(const uint8_t* src_img, uint8_t* dst_img, 
		unsigned int src_width, unsigned int src_height,
		unsigned int dst_width, unsigned int dst_height);

/**
 * @brief according target image scale crop src image, and compress rotate 
 *	for exampe: src image is 1280*720P and target lcd is 320*480, so rotate is 480*320, 
 *  so you should crop the maximul picture to scale compress target picture.
 *	so this algorithns can crop w*h is 1080*720 to scale 480*320p.
 */
int image_scale_crop_compress_rotate(const uint8_t* src_img, uint8_t* dst_img, 
		unsigned int src_width, unsigned int src_height,
		unsigned int dst_width, unsigned int dst_height);

/**
 * @brief according target image scaling src image. 
 */
int image_16bit_scaling(const unsigned char* src_img, unsigned char* dst_img, 
		unsigned int src_width, unsigned int src_height, 
		unsigned int dst_width, unsigned int dst_height);

/**
 * @brief according target image scaling and rotate src image and display in lcd center. 
 */
int image_16bit_scaling_rotate(const unsigned char* src_img, unsigned char* dst_img, 
		unsigned int src_width, unsigned int src_height, 
		unsigned int dst_width, unsigned int dst_height);

/**
 * @brief only crop src image to dst image. 
 */
void image_crop_size(const uint8_t* src_img, uint8_t* dst_img, 
			unsigned int src_width, unsigned int src_height,
			unsigned int dst_width, unsigned int dst_height);

/**
 * @brief anticlockwise rotate90
 */
void image_16bit_rotate90_anticlockwise(uint8_t *des,const uint8_t *src, uint32_t img_width, uint32_t img_height);

/**
 * @brief  clockwise rotate90
 */
void image_16bit_rotate90_clockwise(uint8_t *des, uint8_t *src, uint32_t img_width, uint32_t img_height);





	
/*
 * @}
 */

#ifdef __cplusplus
	}
#endif



