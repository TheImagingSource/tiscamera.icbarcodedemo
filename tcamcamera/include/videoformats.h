#ifndef VIDEOFORMATS_H
#define VIDEOFORMATS_H

#include "videoformat.h"


class CVideoFormats
{
    public:
        void enumerate( GstDevice* device);
        void enumerate( GstElement* tcamsrc);
        bool isRangeType( GstDevice* device);

        int size();
        CVideoFormat &operator[](int i);

    private:
        struct INTPAIR
        {
            int x;
            int y;
        };

        GstElement* openTcamsrc(GstDevice* device);
        void addFormat(GstStructure *str, GstCapsFeatures* features);
        void generateFormats(GstStructure *str, GstElement* tcamsrc, GstCapsFeatures* features);
        void getRangeFps(GstStructure *str, GstElement* tcamsrc,int width, int height, GValue* fpsmin, GValue* fpsmax);
        bool isRangeType( GstElement* tcamsrc);
        bool isValidSize( INTPAIR sz, INTPAIR widthrange, INTPAIR heightrange );
        std::vector<CVideoFormat> _formats;
};

#endif