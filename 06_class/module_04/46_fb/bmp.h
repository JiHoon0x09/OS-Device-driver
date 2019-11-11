/*  BMP파일에 관한 헤더를 선언해 놓았다. */

typedef struct  {
 //     unsigned short    bfType;
        unsigned int      bfSize;
        unsigned short    bfReserved1;
        unsigned short    bfReserved2;
        unsigned int      bfOffBits;
}BITMAPFILEHEADER;

typedef struct {
        unsigned int      biSize;
        unsigned int      biWidth;
        unsigned int      biHeight;
        unsigned short    biPlanes;
        unsigned short    biBitCount;
        unsigned int      biCompression;
        unsigned int      biSizeImage;
        unsigned int      biXPelsPerMeter;
        unsigned int      biYPelsPerMeter;
        unsigned int      biClrUsed;
        unsigned int      biClrImportant;
}BITMAPINFOHEADER;

typedef struct  {
        unsigned char    rgbBlue;
        unsigned char    rgbGreen;
        unsigned char    rgbRed;
        unsigned char    rgbReserved;
}RGBQUAD;

typedef struct {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
}BITMAPINFO;


