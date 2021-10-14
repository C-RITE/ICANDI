#include "VideoWriterVFW.h"

static BITMAPINFOHEADER BitmapHeader(int width, int height, int bpp, int compression = BI_RGB)
{
    BITMAPINFOHEADER bmih;
    memset(&bmih, 0, sizeof(bmih));
    bmih.biSize = sizeof(bmih);
    bmih.biWidth = width;
    bmih.biHeight = height;
    bmih.biBitCount = (WORD)bpp;
    bmih.biCompression = compression;
    bmih.biPlanes = 1;

    return bmih;
}

static void InitAviFileVFW()
{
    static int isInitialized = 0;
    if (!isInitialized)
    {
        AVIFileInit();
        isInitialized = 1;
    }
}

struct BITMAPINFO_8Bit
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
};

/*************************** writing AVIs ******************************/


void VideoWriterVFW::init()
{
    avifile = 0;
    compressed = uncompressed = 0;
    fps = 0;
    tempFrame = NULL;
    pos = 0;
    fourcc = 0;
    isOpenedVar = false;
}

void VideoWriterVFW::close()
{
    if (uncompressed)
        AVIStreamRelease(uncompressed);
    if (compressed)
        AVIStreamRelease(compressed);
    if (avifile)
        AVIFileRelease(avifile);
    tempFrame.release();
    init();
}

bool VideoWriterVFW::isOpened()
{
    return isOpenedVar;
}

bool VideoWriterVFW::open(const char* filename, int _fourcc, double _fps, cv::Size frameSize, bool isColor)
{
    close();

    InitAviFileVFW();

    if (AVIFileOpen(&avifile, filename, OF_CREATE | OF_WRITE, 0) == AVIERR_OK)
    {
        fourcc = _fourcc;
        fps = _fps;
        if (frameSize.width > 0 && frameSize.height > 0 &&
            !createStreams(frameSize, isColor))
        {
            close();
            return false;
        }
        isOpenedVar = true;
        return true;
    }
    else
        return false;
}


bool VideoWriterVFW::createStreams(cv::Size frameSize, bool isColor)
{
    if (!avifile)
        return false;
    AVISTREAMINFO aviinfo;

    BITMAPINFO_8Bit bmih;
    bmih.bmiHeader = BitmapHeader(frameSize.width, frameSize.height, isColor ? 24 : 8);
    for (int i = 0; i < 256; i++)
    {
        bmih.bmiColors[i].rgbBlue = (BYTE)i;
        bmih.bmiColors[i].rgbGreen = (BYTE)i;
        bmih.bmiColors[i].rgbRed = (BYTE)i;
        bmih.bmiColors[i].rgbReserved = 0;
    }

    memset(&aviinfo, 0, sizeof(aviinfo));
    aviinfo.fccType = streamtypeVIDEO;
    aviinfo.fccHandler = 0;
    // use highest possible accuracy for dwRate/dwScale
    aviinfo.dwScale = (DWORD)((double)0x7FFFFFFF / fps);
    aviinfo.dwRate = cvRound(fps * aviinfo.dwScale);
    aviinfo.rcFrame.top = aviinfo.rcFrame.left = 0;
    aviinfo.rcFrame.right = frameSize.width;
    aviinfo.rcFrame.bottom = frameSize.height;

    if (AVIFileCreateStream(avifile, &uncompressed, &aviinfo) == AVIERR_OK)
    {
        AVICOMPRESSOPTIONS copts, * pcopts = &copts;
        copts.fccType = streamtypeVIDEO;
        copts.fccHandler = fourcc != -1 ? fourcc : 0;
        copts.dwKeyFrameEvery = 1;
        copts.dwQuality = 10000;
        copts.dwBytesPerSecond = 0;
        copts.dwFlags = AVICOMPRESSF_VALID;
        copts.lpFormat = &bmih;
        copts.cbFormat = (isColor ? sizeof(BITMAPINFOHEADER) : sizeof(bmih));
        copts.lpParms = 0;
        copts.cbParms = 0;
        copts.dwInterleaveEvery = 0;

        if (fourcc != -1 || AVISaveOptions(0, 0, 1, &uncompressed, &pcopts) == TRUE)
        {
            if (AVIMakeCompressedStream(&compressed, uncompressed, pcopts, 0) == AVIERR_OK &&
                AVIStreamSetFormat(compressed, 0, &bmih, sizeof(bmih)) == AVIERR_OK)
            {
                fps = fps;
                fourcc = (int)copts.fccHandler;
                frameSize = frameSize;
                tempFrame = cv::Mat(frameSize.height, frameSize.width, CV_8UC1);// cvCreateImage(frameSize, 8, (isColor ? 3 : 1));
                return true;
            }
        }
    }
    return false;
}


bool VideoWriterVFW::writeFrame(cv::Mat image, bool isImageFlipped)
{
    bool result = false;

    if (!compressed && !createStreams(cv::Size(image.cols, image.rows), image.channels() > 1))
        return false;

    if (image.cols != tempFrame.cols || image.rows != tempFrame.rows)
        return false;

    if (image.channels() != tempFrame.channels() ||
        image.depth() != tempFrame.depth() ||
        isImageFlipped ||
        image.step[0] != cv::alignSize(image.cols * image.channels() * ((image.depth() & 255) / 8), 4))
    {
        cv::flip(image, tempFrame, 0);
        image = tempFrame;
    }

    if (compressed != 0)
        result = AVIStreamWrite(compressed, pos++, 1, image.data,//imageData,
            image.rows * image.step[0], AVIIF_KEYFRAME, 0, 0) == AVIERR_OK;
    else
        return false;

    return result;
}

VideoWriterVFW* createVideoWriterVFW(const char* filename, int fourcc,
    double fps, cv::Size frameSize, int isColor)
{
    VideoWriterVFW* writer = new VideoWriterVFW;
    if (writer->open(filename, fourcc, fps, frameSize, isColor != 0))
        return writer;
    delete writer;
    return 0;
}
