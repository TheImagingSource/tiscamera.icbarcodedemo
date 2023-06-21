#ifndef VIDEOFORMAT_H
#define VIDEOFORMAT_H

#include <vector>
#include <string>
#include <jsoncpp/json/json.h>
#include <gst/gst.h>

/**********************************************************************
	CFrameRate
	Holds a single frame rate, consisting of numerator and denumerator.
	
*/
class CFrameRate
{
	public:
		CFrameRate()
		{
			_numerator = 0;
			_denominator = 1;
		}

		CFrameRate(const CFrameRate &src)
		{
			_numerator = src._numerator;
			_denominator = src._denominator;
		}

		
		CFrameRate( int numerator, int denominator);
		CFrameRate& operator=(const CFrameRate &src)
		{
			_numerator = src._numerator;
			_denominator = src._denominator;
			return *this;
		}
		
		bool operator!=(const CFrameRate &Rate);
		bool operator==(const CFrameRate &Rate);

		
		std::string ToString();
		int _numerator;
		int _denominator;
		
		Json::Value getJson()
		{
			Json::Value f;
			f["denominator"] = _denominator;
			f["numerator"] = _numerator;
			return f;
		}


		void setJson(Json::Value &fps)
		{
			_denominator = fps["denominator"].asInt();
			_numerator = fps["numerator"].asInt();
		}

};

////////////////////////////////////////////////////////////////
/*
	CVideo Format

	Hold information about width, height and format type of a video 
	format. 
	Also save a list of related frame rates 
*/
class CVideoFormat
{
	public:
		CVideoFormat();
		CVideoFormat(int width, int height);
		CVideoFormat(std::string type, std::string format, 
					std::string binning, std::string skipping, 
					int width, int height, 
					int stepwidth = 16, int stepheight = 4, 
					int widthmin = 1, int heightmin = 1);

		CVideoFormat(const GstStructure *pStructure);
		CVideoFormat(const GstStructure *pStructure,int width, int height, GValue *fpsmin, GValue *fpsmax);

		~CVideoFormat();

		CVideoFormat(const CVideoFormat &src)
		{
			Copy(src);
		};
		
		CVideoFormat& operator=(const CVideoFormat &src)
		{
			Copy(src);
			return *this;
		};
		 
		bool operator!=(const CVideoFormat &Format);
		bool operator==(const CVideoFormat &Format);		
		
		const char* Format(){return _format.c_str();}; 
		const char* Type(){return _type.c_str();}; 
		bool isBinning(){ return _binning != "" && _binning != "1x1";}
		bool isSkipping(){ return _skipping != "" && _skipping != "1x1";}
		const char* Binning(){return _binning.c_str();}; 
		const char* Skipping(){return _skipping.c_str();}; 

		int Width(){return _width;};
		int Height(){return _height;};
		int WidthMin(){return _widthmin;};
		int HeightMin(){return _heightmin;};
		int WidthStep(){return _widthstep;};
		int HeightStep(){return _heightstep;};

		std::string getType(){return _type;};
		std::string getFormat(){return _format;};
		std::string getBinning(){ return _binning;};
		std::string getSkipping(){ return _skipping;};

		void setWidth( int width ){ _width = width;};
		void setHeigth( int height){ _height = height;};



		std::vector<CFrameRate> getFrameRates(){return _framerates;};

		bool isValid(){return _isValid;};

		void debugprint();
		
		const std::string ToString();
		char* getGSTParameter(int FrameRateIndex);

		Json::Value getJson();
		void setJson(Json::Value &videoformat);

		std::vector<CFrameRate> _framerates;

	private:
		std::string _type;
		std::string _format;
		std::string _binning;
		std::string _skipping;

		int _width;
		int _height;
		// The following are needed for the customize format dialog
		int _widthmin;	
		int _heightmin;
		int _widthstep;
		int _heightstep;

		char _Parameter[255];
		bool _isValid;
		GValue *_fpsmin;
		GValue *_fpsmax;

		void create(const GstStructure *pStructure);
		void Copy( const CVideoFormat &src)
		{
			_width = src._width;
			_height = src._height;
			_type = src._type;
			_format = src._format;
			_binning = src._binning;
			_skipping = src._skipping;
			
			_widthmin = src._widthmin;	
			_heightmin = src._heightmin;
			_widthstep = src._widthstep;
			_heightstep = src._heightstep;

			_framerates = src._framerates;
		}
};
#endif