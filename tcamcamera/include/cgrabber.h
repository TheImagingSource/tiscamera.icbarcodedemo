/*
 * Copyright 2017 bvtest <email>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef CGRABBER_H
#define CGRABBER_H

#include <glib.h>
#include <gst/gst.h>
#include <tcam-property-1.0.h>
#include "videoformat.h"
#include "videoformats.h"
#include <string>
#include <vector>
#include <functional>
#include <jsoncpp/json/json.h>
#include <gst/app/gstappsink.h>
#include "utils.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#include <QImage>


typedef gboolean (*BUFFER_CALLBACK_T) (GstElement *image_sink, GstBuffer *buffer, GstPad *pad, void *appdata);

// TODO
#define TcamProp GstElement

class CGrabber
{
	public:
		CGrabber();
		~CGrabber();
		bool OpenCamera(GstDevice *device );
		void CloseCamera();
		static GstDevice *getDeviceBySerial( std::string serial);
		bool EnumerateVideoFormats();
		const std::string getDeviceName(){return _DeviceName;};
		const std::string getSerialNumber(){return _SerialNumber;};
		
		void SetWindowHandle( long unsigned int Handle) {
			_WindowHandle = Handle;
		}
		
		std::string GetLastErrorText(){ return _LastErrorText;};
		void setLastErrorText( const char* errortext ){  _LastErrorText = errortext;};
		
		TcamProp* getTcamBin(){ return (TcamProp*)_TcamBin;};
		  
		bool startLive();
		void stopLive();
		bool restartLive();
		bool isLive(){ return _isLive;};
		bool isDevValid();

		int getImageWidth();
		int getImageHeight();
		//std::vector <CVideoFormat> VideoFormats(){return _VideoFormats;};
		CVideoFormats &VideoFormats(){return _VideoFormats;};
		void set_property_int(const char *Property, const gint64 Value );
		void set_property_double(const char *Property, const double Value );
		void set_property_boolean(const char *Property, const bool  Value );
		bool get_property_boolean(const char *Property, bool *value );
		void set_property_camera_enum(const char *Property, const char* Value );
		void set_property_button(const char *Property );
		bool isPropertyAvailable( const char *Property );
		bool getPropertyRange(const char* Property, gint64* Min, gint64* Max);
		bool getPropertyValueInt(const char* Property, gint64* Value);
		void enableOverlay( bool bEnable);
		bool getOverlayEnabled(){return _OverlayEnabled;}
		std::string getEnumProperty(const char *Property);
		GSList*  getEnumPropertyValues(const char *Property);
		void setEnumProperty(const char *Property, const char* value);
		bool showDeviceSelectionDlg();
		bool showPropertyDlg();
		void updateOverlay();

		Json::Value getJson();
		bool setJson(Json::Value &cam);

		bool getTriggerMode();


		static std::vector<std::string> enumCameras();
		void setWindowSize(int width, int height);

		// Save and load properties only
		bool saveDeviceStatetoFile( const  char* filename);
		bool loadDeviceStateFromFile( const  char* filename);

		// save and load device, format, fps and properties
		void saveDevice(std::string FileName);
		void loadDevice(std::string FileName);

		void assignLoop(GMainLoop* loop ){_loop = loop;};
		GMainLoop* getMainLoop(){ return _loop;};

   		void set_new_frame_callback(std::function< GstFlowReturn(GstAppSink *appsink, gpointer data )>callback, gpointer data);

		CVideoFormat _VideoFormat;
		CFrameRate _FrameRate;
		
		bool snapImage(QImage &qimage, int timeout);

		void SetOverlayGraphicString( const char* data);

	private:
		static GstFlowReturn AppsinkCallback(GstAppSink *appsink, gpointer user_data);
		
		bool LoadPipelineFromFile(const char* PipelineFileName);
		void queryChildElements();
		bool createTcamBin();
		GstElement *gettcambinchild(std::string childname);
		void setPropertiesJson(Json::Value &cam);
		GstDevice *_device;
		std::string _SerialNumber; 
		std::string _DeviceName;
		//std::vector <CVideoFormat> _VideoFormats;
		CVideoFormats _VideoFormats;

		std::string _PipelineFilename;
		bool _isLive;
		bool _OverlayEnabled;
		
		GstElement *_pipeline;
		GstElement *_TcamBin;
		GstElement *_Overlay;
		long unsigned int _WindowHandle;
		

		GMainLoop* _loop;
		GstBus* _bus;
		guint _bus_watch_id;

		GThread *_ps_thread;
		TcamProp* _Properties;

		BUFFER_CALLBACK_T _cbfunc;
		void *_CallbackData;

		std::function<GstFlowReturn(GstAppSink *appsink, gpointer data)> _external_callback;
		gpointer _external_callback_data = nullptr;


		std::string _LastErrorText;
		int WaitForChangeState(GstState newState);
		bool link_tcambin(bool link);		
		static void* thread_function(void *data);
		bool setCaps();
		int setWindowHandle_int();

		std::string _PipeLineString;
		bool _snapImage = false;
		QImage *_pQImageBuffer;
};

#endif // CGRABBER_H
