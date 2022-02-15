#ifndef AT_VIDEO_COMMON_H_
#define AT_VIDEO_COMMON_H_

#include "at_common.h"

//#define SET_PICTURE_SAVE_PATH(i)        os_printf("./image/%d.txt", i)

typedef struct {
	/// the frame id
	UINT8 id;
	/// the flag of end frame, 1 for end
	UINT8 is_eof;
	/// the packet count of one frame
	UINT8 pkt_cnt;
	/// the packet header's count of one frame
	UINT8 pkt_seq;
}vbuf_header_t;

typedef struct {
	/// the video data receive complete
	beken_semaphore_t aready_semaphore;
	/// the receive video data, malloc by user
	UINT8 *buf_base;  // handler in usr thread
	/// video buff length, malloc by user
	UINT32 buf_len;
	/// frame id
	UINT32 frame_id;
	/// the packet count of one frame
	UINT32 frame_pkt_cnt;
	/// recoder the buff ptr of every time receive video packte
	UINT8 *buf_ptr;
	/// the length of receive one frame
	UINT32 frame_len;
	/// video buff receive state
	UINT32 receive_state;
	/// write file addr
	UINT32 file_ptr;
}video_buff_t;

enum video_buff_state {
	/// video buff init
	VIDEO_BUFF_IDLE = 0,
	/// video buff begin copy
	VIDEO_BUFF_COPY,
	/// video buff full
	VIDEO_BUFF_FULL,
};

#endif

