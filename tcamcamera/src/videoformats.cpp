#include "videoformats.h"

/////////////////////////////////////////////////////////////////////
// Get the video formats of the passed GstDevice
//
void CVideoFormats::enumerate( GstDevice* device)
{
    _formats.clear();
    GstElement *tcamsrc = openTcamsrc(device);
    if( tcamsrc == nullptr)
    {

        printf("\x1B[31mVideoformats: Failed to get tcamsrc\x1B[0m\n");
        return;
    }
    enumerate(tcamsrc);

   	gst_element_set_state( tcamsrc, GST_STATE_NULL);
    gst_object_unref(tcamsrc);
}

/////////////////////////////////////////////////////////////////////
// Call this, if the device is already opened.
// Enumerate or generate the video format list.
//
void CVideoFormats::enumerate( GstElement* tcamsrc)
{
    bool isRange = isRangeType(tcamsrc);
    

	GstPad *pSourcePad  = gst_element_get_static_pad(tcamsrc,"src");
    if( pSourcePad != nullptr)
    {
        GstCaps* pad_caps = gst_pad_query_caps( pSourcePad, NULL );

        if( pad_caps != NULL )
        {
            for (int i = 0; i < gst_caps_get_size (pad_caps); i++)
            {
                GstStructure *str = gst_caps_get_structure (pad_caps, i);
                GstCapsFeatures* features = gst_caps_get_features(pad_caps, i);
                if (gst_structure_get_field_type(str, "format") == G_TYPE_STRING)
                {
                    if( gst_structure_get_field_type(str, "width") == GST_TYPE_INT_RANGE && isRange)
                        generateFormats(str, tcamsrc, features);
                    else
                        if( gst_structure_get_field_type(str, "width") != GST_TYPE_INT_RANGE && !isRange)
                            addFormat(str, features);
                }
                else
                {
                    printf("\x1B[33munknown format type %s\n",G_VALUE_TYPE_NAME( gst_structure_get_field_type(str, "format") ));
                }
            }
            gst_caps_unref(pad_caps);
        }
        g_object_unref(pSourcePad);
    }


}


///////////////////////////////////////////////////////////
// Open tcamsrc associated with the device.
//
GstElement* CVideoFormats::openTcamsrc(GstDevice* device)
{
    if( device == nullptr)
    {
        printf("\x1B[31mVideoformats: GstDevice is null\x1B[0m\n");
        return nullptr;
    }

    GstStructure* devstruc = gst_device_get_properties( device );
    GstElement* TcamBin = gst_element_factory_make("tcamsrc", "source");
	g_object_set(TcamBin,"tcam-device",device,NULL);

	gst_element_set_state(TcamBin, GST_STATE_READY);
	GstStateChangeReturn Result = gst_element_get_state( TcamBin, NULL, NULL, 4000000000ll);

    if( Result !=  GST_STATE_CHANGE_SUCCESS  )
    {
        gst_element_set_state(TcamBin, GST_STATE_NULL);
        gst_object_unref(TcamBin);
        return nullptr;
    }   
    return TcamBin;
}

////////////////////////////////////////////////////////////////////
// Query wether the device supports resolution ranges
//
bool CVideoFormats::isRangeType( GstElement* tcamsrc)
{
    GstPad *pSourcePad  = gst_element_get_static_pad(tcamsrc,"src");
    if( pSourcePad != nullptr)
    {
        GstCaps* pad_caps = gst_pad_query_caps( pSourcePad, NULL );

        if( pad_caps != NULL )
        {
            for (int i = 0; i < gst_caps_get_size (pad_caps); i++)
            {
                GstStructure *str = gst_caps_get_structure (pad_caps, i);
                GstCapsFeatures* features = gst_caps_get_features(pad_caps, i);
                if (gst_structure_get_field_type(str, "format") == G_TYPE_STRING)
                {
                    if( gst_structure_get_field_type(str, "width") == GST_TYPE_INT_RANGE)
                    {
                        gst_caps_unref(pad_caps);            
                        g_object_unref(pSourcePad);
                        return true;
                    }
                }
                else
                {
                    printf("\x1B[33munknown format type %s\n",G_VALUE_TYPE_NAME( gst_structure_get_field_type(str, "format") ));
                }
            }
            gst_caps_unref(pad_caps);
        }
        g_object_unref(pSourcePad);
    }
    return false;
}

////////////////////////////////////////////////////////////////////
// Query wether the device supports resolution ranges, public version
//
bool CVideoFormats::isRangeType( GstDevice* device)
{
    GstElement *tcamsrc = openTcamsrc(device);
    bool result = isRangeType(tcamsrc);
    gst_element_set_state( tcamsrc, GST_STATE_NULL);
    gst_object_unref(tcamsrc);
    return result;
}


/////////////////////////////////////////////////////////////
// Add a new video format from the passed structure
//
void CVideoFormats::addFormat(GstStructure *str, GstCapsFeatures* features)
{
    if( strcmp( gst_structure_get_name(str),"ANY") != 0 )
    {
        if (features)
        {
            if (gst_caps_features_contains(features, "memory:NVMM"))
            {
                // do something with this information
            }
        }

        CVideoFormat vf(str);
        if( vf.isValid() )
            _formats.push_back(vf);
    }
}

#define ARRAY_SIZE(array) \
    (sizeof(array) / sizeof(array[0]))

///////////////////////////////////////////////////////////////////////////////
//
void CVideoFormats::getRangeFps(GstStructure *str, GstElement* tcamsrc,int width, int height, GValue* fpsmin, GValue* fpsmax)
{
    GstCaps* caps = gst_caps_new_empty();

    GstStructure* structure = gst_structure_copy(str);
	gst_structure_set(structure, 
							"width", G_TYPE_INT,width,
							"height", G_TYPE_INT,height,
							NULL); 
    gst_structure_remove_field(structure, "framerate");
    gst_caps_append_structure(caps, structure);	
//    printf("%s\n", gst_caps_to_string(caps));

    GstQuery* query = gst_query_new_caps (caps);

    gboolean ret = gst_element_query(tcamsrc, query);
    if (ret)
    {
        GstCaps* result_caps = NULL;
        gst_query_parse_caps_result(query, &result_caps);

        GstStructure *s = gst_caps_get_structure( result_caps, 0);

        const GValue *value = gst_structure_get_value (s, "framerate"); 

        g_value_copy( gst_value_get_fraction_range_min(value), fpsmin );
        g_value_copy( gst_value_get_fraction_range_max(value), fpsmax );

        gst_value_set_fraction(fpsmin,
                                gst_value_get_fraction_numerator( gst_value_get_fraction_range_min(value)),
                                gst_value_get_fraction_denominator(gst_value_get_fraction_range_min(value))
                               );

        gst_value_set_fraction(fpsmax,
                                gst_value_get_fraction_numerator( gst_value_get_fraction_range_max(value)),
                                gst_value_get_fraction_denominator(gst_value_get_fraction_range_max(value))
                               );

        //printf("%d / %d \n",gst_value_get_fraction_numerator( fpsmin) ,gst_value_get_fraction_denominator(fpsmin));
        // result_caps will contain the allowed framerates
        // video/x-bayer,format=rggb,width=1920,height=1080,framerate={75/1,30/1}
        //const char* str = gst_caps_to_string(result_caps);
        //printf("Caps are: %s\n", str);
        //gst_structure_get_int(result_caps,)
        //g_free((gpointer)str);
        // result caps are still owned by query

    }
    else
    {
        // query could not be performed
    }
    gst_query_unref(query);
}

void CVideoFormats::generateFormats(GstStructure *str, GstElement* tcamsrc, GstCapsFeatures* features)
{
    const char* name = gst_structure_get_name(str);

    const char* format = gst_structure_get_string(str, "format");

    printf("%s %s{ ", name, format); 
    std::string binning;
    std::string skipping;

   	const char* b = gst_structure_get_string(str, "binning");
	if( b != nullptr)
    {
        printf("[Binning %s] ",b);
        binning = b;
    }

	const char* s = gst_structure_get_string(str, "skipping");
	if( s != nullptr)
    {
        printf("[Skipping %s] ",s);
        skipping = s;
    }

    const GValue* val = gst_structure_get_value(str, "format");

    INTPAIR width;


    width.x = gst_value_get_int_range_min(gst_structure_get_value(str, "width"));
    width.y = gst_value_get_int_range_max(gst_structure_get_value(str, "width"));
    int width_step = gst_value_get_int_range_step(gst_structure_get_value(str, "width"));


    printf("width: [%d-%d : %d]", width.x, width.y, width_step);

    INTPAIR height;
    height.x = gst_value_get_int_range_min(gst_structure_get_value(str, "height"));
    height.y = gst_value_get_int_range_max(gst_structure_get_value(str, "height"));
    int height_step = gst_value_get_int_range_step(gst_structure_get_value(str, "height"));


    printf("height: [%d-%d : %d]", height.x, height.y, height_step);
    printf("\n");


	INTPAIR sizePrefs[] = 
        {
            {320, 240},
            {640, 480},
            {1024, 786}, 
            {1280, 720}, 
            {1920, 1080}, 
            {1600, 1200}, 
            {2048, 1536},
            {2560, 1920}
        };
    //printf("Reses %d\n", ARRAY_SIZE(sizePrefs));

    printf("\t(%dx%d}  ",width.x,height.x);

    GValue minfps = G_VALUE_INIT;
    GValue maxfps = G_VALUE_INIT;

    g_value_init (&minfps, GST_TYPE_FRACTION);
    g_value_init (&maxfps, GST_TYPE_FRACTION);
   

    getRangeFps(str,tcamsrc, width.x,height.x, &minfps, &maxfps);
    double max = (double)gst_value_get_fraction_numerator(&maxfps) / (double)gst_value_get_fraction_denominator(&maxfps);
    printf("%f fps\n",max);


    CVideoFormat vf1(str, width.x,height.x, &minfps, &maxfps);
    if( vf1.isValid() )
        _formats.push_back(vf1);

    for( auto sz : sizePrefs )
    {
        if( isValidSize( sz, width, height ) )
        {
  //          printf("\t(%dx%d}\n",sz.x, sz.y);            
            getRangeFps(str,tcamsrc, sz.x,sz.x, &minfps, &maxfps);
            max = (double)gst_value_get_fraction_numerator(&maxfps) / (double)gst_value_get_fraction_denominator(&maxfps);
            printf("%f fps\n",max);

            CVideoFormat vf(str, sz.x,sz.y, &minfps, &maxfps);
            if( vf.isValid() )
                _formats.push_back(vf);
        }
    }

    //printf("\t(%dx%d}\n",width.y,height.y);
    getRangeFps(str,tcamsrc, width.y,height.y, &minfps, &maxfps);
    max = (double)gst_value_get_fraction_numerator(&maxfps) / (double)gst_value_get_fraction_denominator(&maxfps);
    printf("%f fps\n",max);

    CVideoFormat vf2(str, width.y,height.y, &minfps, &maxfps);
    if( vf2.isValid() )
        _formats.push_back(vf2);

}

bool CVideoFormats::isValidSize( INTPAIR sz, INTPAIR widthrange, INTPAIR heightrange )
{
    return( widthrange.x < sz.x && sz.x < widthrange.y &&
            heightrange.x < sz.y && sz.y < heightrange.y );
}

int CVideoFormats::size()
{
    return (int)_formats.size();
}

CVideoFormat &CVideoFormats::operator[](int i)
{
    return _formats[i];
};
