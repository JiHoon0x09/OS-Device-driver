#ifndef _OSS_H_
#    define _OSS_H_



#include "gtype.h"


struct oss_desc 
{
    int    aout_frags;     // 전체 fragment의 갯수
    int    aout_fragsize;  // 한 fragment의 크기(in bytes)   
    
    uint32 aout_tsize;     // total fragments's bytes(= total dmabuffer size)
	int    aout_bytesps;   // bytes per second, (channels * (8비트냐 16비트냐에 따라 1이냐 2냐 틀려짐) * (sampling freq) )/8
	int    aout_freq;      // sampling rate
	int    aout_channels;  // channel, 1 or 2
	int    aout_format;    // AFMT_S16_LE 등과 같은 audio format number 
};


/* linux/soundcard.h, audio format 
#	define AFMT_U8			0x00000008
#	define AFMT_S16_LE		0x00000010	// Little endian signed 16
#	define AFMT_S16_BE		0x00000020	// Big endian signed 16 
#	define AFMT_S8			0x00000040
#	define AFMT_U16_LE		0x00000080	// Little endian U16 
#	define AFMT_U16_BE		0x00000100	// Big endian U16 
*/


struct oss_set
{
    int frags;     // 전체 fragment의 갯수
    int fragsize;  // 한 fragment의 크기(in bytes)   
	int freq;
	int channels;
	int format;
};


int           oss_init(struct oss_set *os);
void          print_oss_desc(struct oss_desc *od, struct oss_set *os);
inline int    get_audio_dma_buffer_freespace(void);
inline uint32 get_bytes_in_dma_buffer(void);
void          oss_close(void);
inline int    oss_write(uint8 *dptr, int size);
inline void   oss_reset(void);
inline float get_delay_in_dma_buffer(void);

extern struct oss_desc od;

#endif
