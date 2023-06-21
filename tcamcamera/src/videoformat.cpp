#include "videoformat.h"
#include "fmt/core.h"

CVideoFormat::CVideoFormat()
{

	_width = -1;
	_height = -1;
	_binning = "1x1";
	_skipping = _binning;

	_fpsmin = nullptr;
	_fpsmax = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////
// This constructor is used, if the video format is created from a resolution 
// ranges
CVideoFormat::CVideoFormat(const GstStructure *pStructure,int width, int height, GValue *fpsmin, GValue *fpsmax)
{
	_fpsmin = fpsmin;
	_fpsmax = fpsmax;

	create(pStructure);
	_width = width;
	_height = height;
	

	// Query min resolution. Needed for the customize format dialog
	_widthmin = gst_value_get_int_range_min(gst_structure_get_value(pStructure, "width"));
	_heightmin = gst_value_get_int_range_min(gst_structure_get_value(pStructure, "height"));
}

CVideoFormat::CVideoFormat(const GstStructure *pStructure)
{
	_fpsmin = nullptr;
	_fpsmax = nullptr;

	create(pStructure);
}

CVideoFormat::CVideoFormat(std::string type, std::string format, 
					std::string binning, std::string skipping, 
					int width, int height, 
					int stepwidth, int stepheight, 
					int widthmin, int heightmin)
{
		_binning = "1x1";
		_skipping = _binning;

		_width = width;
		_height = height;
		_type = type;
		_format = format;
		if( binning != "")
			_binning = binning;
		if( _skipping != "")	
			_skipping = skipping;
		_width = width;
		_height = height;

		_widthmin = widthmin;	
		_heightmin = heightmin;
		_widthstep = stepwidth;
		_heightstep = stepheight;
		_isValid = true;
		_fpsmin = nullptr;
		_fpsmax = nullptr;
}

CVideoFormat::CVideoFormat(int width, int height)
{
	_binning = "1x1";
	_skipping = _binning;
	_width = width;
	_height = height;
	_widthmin = 1;	
	_heightmin = 1;
	_widthstep = 16;
	_heightstep = 4;
}


///////////////////////////////////////////////////////////////////////////////
// Create the video format from the structure.
// min width and height are not set, because we have range formats here
//
void CVideoFormat::create(const GstStructure *pStructure)
{
	const char * buffer = gst_structure_get_string (pStructure, "format" );
	const char* b = gst_structure_get_string(pStructure, "binning");
	if( b != nullptr)
		_binning = b;

	const char* s = gst_structure_get_string(pStructure, "skipping");
	if( s != nullptr)
		_skipping = s;

	_isValid = false;
	if( buffer == NULL )
	{
		_format = "Unknown";
	}
	else
	{
		if( strlen(buffer) != 0 )
		{
			_format = buffer ;
		
			_type = gst_structure_get_name(pStructure);
			gst_structure_get_int (pStructure, "width", &_width);
			gst_structure_get_int (pStructure, "height", &_height);
			_widthstep = gst_value_get_int_range_step(gst_structure_get_value(pStructure, "width"));
			_heightstep = gst_value_get_int_range_step(gst_structure_get_value(pStructure, "height"));

//			if( _width == 1920 && _height == 1080)
//				printf("%s %s B%s S%s (%dx%d)\n", Type(), Format(), Binning(), Skipping(), Width(), Height() );


			// Query the frame rate list of this video format.
			const GValue *value = gst_structure_get_value (pStructure, "framerate"); 
			if (G_VALUE_TYPE(value) == GST_TYPE_LIST)
			{
				for( int i = 0; i < gst_value_list_get_size(value); ++i )
				{
					const GValue *test = gst_value_list_get_value(value,i);
					if( G_VALUE_TYPE(test) == GST_TYPE_FRACTION)
					{
						_isValid = true;
						auto num = gst_value_get_fraction_numerator(test);
						auto den = gst_value_get_fraction_denominator(test);
						_framerates.push_back( CFrameRate(num, den ));
						if( _width == 1920 && _height == 1080)
						{
							int x = 0;
							gst_structure_get_int (pStructure, "width", &x);
							//printf("\t%d\t%6d/%6d\n",x,num,den);
						}
					}
				}
			}
			else if (G_VALUE_TYPE(value) == GST_TYPE_FRACTION_RANGE)
			{
				_framerates.push_back(CFrameRate(gst_value_get_fraction_numerator(_fpsmin), gst_value_get_fraction_denominator(_fpsmin)) );
				double min = (double)gst_value_get_fraction_numerator(_fpsmin) / (double)gst_value_get_fraction_denominator(_fpsmin);
				double max = (double)gst_value_get_fraction_numerator(_fpsmax) / (double)gst_value_get_fraction_denominator(_fpsmax);
				int start =  ((int)(min / 10.0 ) + 1) * 10;
				int end = ((int)(max / 10.0 ) ) * 10;
				end = (int)max;

				int fps = (int)(min +1);
				while( fps < (int)max)
				{
					if( fps < 20 )
					{
						if( fps < end ) _framerates.push_back(CFrameRate(fps, 1)) ;	
						fps++;
					}else if( fps < 50 )
					{						
						if( fps < end ) _framerates.push_back(CFrameRate(fps, 1)) ;	
						fps+=5;
					}
					else if( fps < 150 )
					{
						fps+=10;
						if( fps < end ) _framerates.push_back(CFrameRate(fps, 1)) ;	
					}
					else if( fps < 300 )
					{
						fps+=50;
						if( fps < end ) _framerates.push_back(CFrameRate(fps, 1)) ;	
					}
					else 
					{
						fps+=100;
						if( fps < end ) _framerates.push_back(CFrameRate(fps, 1)) ;	
					}
				}
				_framerates.push_back(CFrameRate(gst_value_get_fraction_numerator(_fpsmax), gst_value_get_fraction_denominator(_fpsmax)) );
				_isValid = true;
			}
		}
	}
}

CVideoFormat::~CVideoFormat()
{
	_framerates.clear();
}

///////////////////////////////////////////////////////////
// return a readable string for lists etc.
//
const std::string CVideoFormat::ToString()
{
	std::string readable = fmt::format("{0} ({1}x{2})", _format, _width, _height);

	if( _binning != "1x1" && _binning != "")
		readable += fmt::format(" [Binning {0}]", _binning);

	if( _skipping != "1x1" && _skipping != "")
		readable += fmt::format(" [Skipping {0}]", _skipping);

	//printf("%s\n",readable.c_str());
	return readable;
}

char* CVideoFormat::getGSTParameter(int FrameRateIndex)
{
	sprintf(_Parameter,"%s,format=%s,binning=%s,skipping=%s,width=%d,height=%d,framerate=%d/%d",_type.c_str(), _format.c_str(),
			_binning.c_str(),_skipping.c_str(),
			_width, _height, _framerates[FrameRateIndex]._numerator,_framerates[FrameRateIndex]._denominator);
	printf("%s\n",_Parameter);
	return _Parameter;
}

bool CVideoFormat::operator!=(const CVideoFormat &Format)
{
	// TODO add binning & skipping
	return _height != Format._height || _width != Format._width ||
			 _format != Format._format || _binning!=Format._binning || _skipping!=Format._skipping;
}

bool CVideoFormat::operator==(const CVideoFormat &Format)
{
	// TODO add binning & skipping
	return _height == Format._height && _width == Format._width && 
		_format==Format._format && _binning==Format._binning && _skipping==Format._skipping;
}

void CVideoFormat::debugprint()
{
	printf("Videoformat : %s %s b%s s%s %d %d\n",_type.c_str(), _format.c_str(),
						_binning.c_str(),_skipping.c_str(), _width, _height);
}


///////////////////////////////////////////////////////////////////////////////
// Json im and export
Json::Value CVideoFormat::getJson()
{
	Json::Value v;
	v["format"] = _format;
	v["type"] = _type;
	v["binning"] = _binning;
	v["skipping"] = _skipping;
	v["width"] = _width;
	v["height"] = _height;
	return v;
}

void CVideoFormat::setJson(Json::Value &videoformat)
{
	_width = videoformat["width"].asInt();
	_height = videoformat["height"].asInt();
	_type =  videoformat["type"].asString();
	_format=   videoformat["format"].asString();
	if(videoformat.isMember("binning"))
		_binning =   videoformat["binning"].asString();
	if(videoformat.isMember("skipping"))
		_skipping =   videoformat["skipping"].asString();
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

CFrameRate::CFrameRate( int numerator, int denominator)
{
	_numerator = numerator;
	_denominator = denominator;
}

std::string CFrameRate::ToString()
{
	return fmt::format("{0} fps", (double)_numerator / (double)_denominator);
}


bool CFrameRate::operator!=(const CFrameRate &Rate)
{
	return _denominator != Rate._denominator || _numerator != Rate._numerator;
}

bool CFrameRate::operator==(const CFrameRate &Rate)
{
	return _denominator == Rate._denominator && _numerator == Rate._numerator;
}
