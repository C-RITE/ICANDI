#ifndef _video_writer_vfw_H
#define _video_writer_vfw_H

#include <windows.h>
#include <vfw.h>
#include <opencv2/core.hpp> 
#include <opencv2/videoio.hpp>

class VideoWriterVFW
{
public:
    VideoWriterVFW() { init(); }
    virtual ~VideoWriterVFW() { close(); }

    virtual bool open(const char* filename, int fourcc,
        double fps, cv::Size frameSize, bool isColor);
    virtual void close();
    virtual bool writeFrame(cv::Mat, bool isImageFlipped);
    virtual bool isOpened();

protected:
    void init();
    bool createStreams(cv::Size frameSize, bool isColor);

    PAVIFILE      avifile;
    PAVISTREAM    compressed;
    PAVISTREAM    uncompressed;
    double        fps;
    cv::Mat tempFrame;
    long          pos;
    int           fourcc;
    bool          isOpenedVar;
};

#endif