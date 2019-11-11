#include "oss.h"


#define DEBUG
#    ifdef DEBUG
#        define gprintf(x...) printf(x)
#    else
#        define gprintf(x...)
#    endif



/* linux/soundcard.h, audio format 
#	define AFMT_U8			0x00000008
#	define AFMT_S16_LE		0x00000010	// Little endian signed 16
#	define AFMT_S16_BE		0x00000020	// Big endian signed 16 
#	define AFMT_S8			0x00000040
#	define AFMT_U16_LE		0x00000080	// Little endian U16 
#	define AFMT_U16_BE		0x00000100	// Big endian U16 
*/

static   audio_buf_info  ainfo;
static   int             audio_fd;

struct oss_desc od;

/*
    int frags;     // �������� ���� ���� 0���� �Ѵ�.
    int fragsize;  // �������� ���� ���� 0���� �Ѵ�.
	int freq;
	int channels;
	int format;
*/
int oss_init(struct oss_set *os)
{
	int arg;
	int fsize_calc, bps; // frag size ���� ����
	
	
    audio_fd = open("/dev/dsp", O_WRONLY, 0);
    if (audio_fd < 0)
    {
        printf("Cannot open /dev/dsp ==> open /dev/sound/dsp \n");
	    audio_fd = open("/dev/sound/dsp", O_WRONLY, 0); // devfs����
	    if (audio_fd < 0)
    	{
        	printf("Cannot open /dev/sound/dsp\n");
        	return -1;
    	}
    }

    if( ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &arg) < 0 )
    {
        printf("cannot get audio format");
        goto fail;
    }

    gprintf("current driver's oss format = 0x%08x\n", arg);
    
    /* s3c2410/s3c2440/mmsp2 must be AFMT_S16_LE ^^ */
    if( os->format != arg )
    {
	   	// set audio format
    	arg = os->format;
		gprintf("os->format != arg is happened  arg =  0x%08x, os->format = 0x%08x\n", arg, os->format);
    	
    	// return���� ���� ���� ���ƾ� �Ѵ�.
    	if( (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &arg) < 0) || (arg != os->format) )
    	{
        	printf("cannot set audio format");
        	goto fail;
    	}
    }
    
    os->format = arg; // �缼��

	// set channel
    arg = os->channels;
    if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &arg) < 0)
    {
        arg = (os->channels > 1)? 1 : 0; // 1�̸� stereo, 0�̸� mono
        if( ioctl(audio_fd, SNDCTL_DSP_STEREO, &arg) < 0 )
        {
        	printf("fail set channels");
        	goto fail;
        }
        // SNDCTL_DSP_STEREO, �� �༮�� mmsp2�� ���� return argument�� ����.
        // �ؼ� �׳� ���� �������ش�.
        arg = (arg ? 2 : 1);
    }

    gprintf("oss stereo channels = %d\n", arg);
    
    os->channels = arg;
    
    gprintf("DDDDDDDDDD os frags = %d, %d\n", os->frags, ((os->frags << 16)>>16) );

	// oss�� frag ������, ���� 16��Ʈ�� fragment ����, ���� 16��Ʈ�� 2�� n�� n�� �ȴ�.
	// ��, 2�� n�� ��ŭ ���۸� ��´�. �ؼ� 64k�̻��� ������ ���Ѵ�.(2�� 16��)
    // ���� 16��Ʈ���� fragment�� ������ ����ְ�, ���� 16��Ʈ���� fragment�� size��
    // ����. ���� 16��Ʈ�� 2^n ���¿��� n���� ���� ���� n�� 15�� ���� 2^15�� �ǰ�����
	if( os->frags  || os->fragsize ) // frag���� ���� Ȥ�� frag size������ ���� ���	
	{
		fsize_calc = 0;

		if( os->fragsize ) // frag size ������ ���, 
		{
			// 2^n ������ frag size�� �����Ѵ�.
    		for (fsize_calc = 0; (0x01 << fsize_calc) < os->fragsize; ++fsize_calc);
    	}
    	
    	if( os->frags > 16 ) os->frags = 16; // fragment ���� ����
    	
    	fsize_calc |= (os->frags << 16);    

        // set audio fragment parameter
        gprintf("set audio buffer to %d fragments: %d bytes\n", (fsize_calc >> 16), 1 << (fsize_calc & 0xFFFF) );

    	if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &fsize_calc) < 0)
    	{
        	printf("audio buffer fragments set error ==> no fail, use original driver setting value\n");
    	}
	}
	
    if( ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &ainfo) < 0 )
    {
    	printf("SNDCTL_DSP_GETOSPACE error\n");
    }
    gprintf("fragments  = %d\n", ainfo.fragments);
    gprintf("fragstotal = %d\n", ainfo.fragstotal);
    gprintf("fragsize   = %d\n", ainfo.fragsize);
    gprintf("bytes      = %d\n", ainfo.bytes);
	
	os->frags    = ainfo.fragstotal;
	os->fragsize = ainfo.fragsize;

    arg = os->freq;
    if( ioctl(audio_fd, SNDCTL_DSP_SPEED, &arg) < 0 )
    {
        printf("cannot set audio frequency(sampling rate)\n");
        goto fail;
    }
    os->freq = arg;
    
    od.aout_frags    = ainfo.fragstotal;     // ��ü fragment�� ����
    od.aout_fragsize = ainfo.fragsize;       // �� fragment�� ũ��(in bytes)   
    od.aout_tsize    = ainfo.fragstotal * ainfo.fragsize; // total buffer size in bytes
    //od.aout_tsize    = ainfo.bytes; // ������ ����ִ� �༮�� ũ���...������� ��������
    
    if( (os->format == AFMT_U8 ) || (os->format == AFMT_S8 ) )
    	bps = 1;
    else
    	bps = 2;

	od.aout_bytesps  = (bps * os->channels * os->freq); // channels * (8��Ʈ�� 16��Ʈ�Ŀ� ���� 1�̳� 2�� Ʋ����) * (sampling freq) 	
	od.aout_freq     = os->freq;      // sampling rate
	od.aout_channels = os->channels;  // channel, 1 or 2
	od.aout_format   = os->format;    // AFMT_S16_LE ��� ���� audio format number 
    
    
    gprintf("od out spec: size = %d, tsize/out_bytes_p_sec = %f\n\n\n", od.aout_tsize, (float)od.aout_tsize/(float)od.aout_bytesps);    
    
    return 0;
    
fail:
	close(audio_fd);
	return -1;    
}


void print_oss_desc(struct oss_desc *od, struct oss_set *os)
{
	printf("\nod->aout_frags  = %d\n",   od->aout_frags);     
    printf("od->aout_fragsize = %d\n",   od->aout_fragsize);  
    printf("od->aout_tsize    = %u\n",   od->aout_tsize);  
	printf("od->aout_bytesps  = %d\n",   od->aout_bytesps);       
	printf("od->aout_freq     = %d\n",   od->aout_freq);      
	printf("od->aout_channels = %d\n",   od->aout_channels);  
	printf("od->aout_format   = %d\n\n", od->aout_format);  

    printf("os->frags         = %d\n",   os->frags);     
    printf("os->fragsize      = %d\n",   os->fragsize);  
	printf("os->freq          = %d\n",   os->freq);
	printf("os->channels      = %d\n",   os->channels);
	printf("os->format        = %d\n",   os->format);
}

#if 1 // �̷��� �ϸ� ��縮 ����. ������ ã�Ƴ� ��
inline int get_audio_dma_buffer_freespace(void)
{
	uint32 nbytes;
	
    if( ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &ainfo) < 0 )
    {
    	printf("SNDCTL_DSP_GETOSPACE error\n");
    }
    
    nbytes = (uint32)ainfo.bytes;
    
    if( nbytes >= 65536 ) return 65536;
    
    return ainfo.bytes;
}
#else
inline int get_audio_dma_buffer_freespace(void)
{
  	int playsize=od.aout_fragsize;

  	if(ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &ainfo)!=-1)
  	{
    	// calculate exact buffer space:
    	playsize = ainfo.fragments*ainfo.fragsize;
    	
    	if (playsize > 65536)
    		playsize = (65536 / ainfo.fragsize) * ainfo.fragsize;
    	
    	return playsize;
  	}
	
	return playsize;
}

#endif

inline uint32 get_bytes_in_dma_buffer(void)
{
    if( ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &ainfo) < 0 )
    {
    	printf("SNDCTL_DSP_GETOSPACE error\n");
    }
    return (od.aout_tsize-(uint32)ainfo.bytes); // total size - ����ִ� buffer�� ũ�� = data�� �ִ� ũ��
}



void oss_close(void)
{
	ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
	close(audio_fd);
}


inline int oss_write(uint8 *dptr, int size)
{
	return write(audio_fd, dptr, size);
}
	
inline void oss_reset(void)
{
	ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
}


/*
struct oss_set
{
    int frags;     // ��ü fragment�� ����
    int fragsize;  // �� fragment�� ũ��(in bytes)   
	int freq;
	int channels;
	int format;
};
*/
static struct oss_set oss;


int main(void)
{

    FILE *in;
   
    int  ban=16384, len;
	char buf[44], buf_wav[ban];
	int  count=0;
	
	int i = 0, j;
	
	#if 0
	oss.frags    = 0; // orig /dev/dsp default setting
    oss.fragsize = 0; // orig /dev/dsp default setting 
	oss.freq     = 44100;
	oss.channels = 2;
	oss.format   = AFMT_S16_LE;
	#else
	oss.frags    = 16; // orig /dev/dsp default setting
	//oss.frags    = 0; // orig /dev/dsp default setting  //june modi
    oss.fragsize = 8192; // orig /dev/dsp default setting 
	//oss.fragsize = 16984;  //june modi
	//oss.freq     = 44100; 
	oss.freq     = 22050; //june modi
	oss.channels = 2;
	oss.format   = AFMT_S16_LE;
	#endif
	
	if( oss_init(&oss) < 0 )
	{
		printf("oss setting error\n");
		return -1;
	}
	
    print_oss_desc(&od, &oss);
	
	oss_reset();
	
	//in = fopen("./g.wav", "rb");
	in = fopen("./SoHot.wav", "rb"); //june add
	count = fread(buf, 1, 44, in); // 44bytes wav header�� �׳� �о ����
	//gprintf("44bytes : %s\n", &buf);

	j = 0;
	while( !feof(in) )
	{
		j++;
		
		if( j>200) break;
		//printf("REPLAY=[%02d:%03d] [%d], %u, %10.4f\n", i, j, len, get_bytes_in_dma_buffer(), (float)get_bytes_in_dma_buffer()/(float)od.aout_bytesps );
		len = fread(buf_wav, 1,ban, in);
		if (len >= ban)
		{
		    oss_write(buf_wav, len);
		}
		else
		{
		    len=ban-len;
		    oss_write(buf_wav, len);
		}
		//printf("REPLAY=[%d], %u, %10.4f\n",len, get_bytes_in_dma_buffer(), (float)get_bytes_in_dma_buffer()/(float)od.aout_bytesps );
		
	}
	fclose(in);
	oss_close();
	
	
	return 1;
}


