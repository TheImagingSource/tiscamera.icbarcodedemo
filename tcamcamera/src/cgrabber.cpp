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

#include "cgrabber.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <unistd.h>
#include <memory>
#include <gst/video/videooverlay.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include "cdeviceselectiondlg.h"
#include "cpropertiesdialog.h"


static gboolean bus_call (GstBus* bus, GstMessage* msg, gpointer data)
{
    CGrabber* caller = (CGrabber*) data;


	if( gst_is_video_overlay_prepare_window_handle_message (msg) )
	{
		printf("Set Window handle now!");
	}

    switch (GST_MESSAGE_TYPE (msg))
    {

        case GST_MESSAGE_EOS:
        {
			g_print("bus_call : EOS received.\n");
            g_main_loop_quit (caller->getMainLoop() );
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            gchar*  debug;
            GError* error;

            gst_message_parse_error (msg, &error, &debug);
            g_free (debug);

            g_printerr ("bus_call :  Error:  %d %s\n", error->code, error->message);
			if( error->code == 1)
			{
				caller->restartLive();
			}
			
            g_error_free (error);
            break;
        }
        case GST_MESSAGE_WARNING:
        {
            gchar*  debug;
            GError* Warn;

            gst_message_parse_warning (msg, &Warn, &debug);
			//caller->setLastErrorText( (const char*) debug);

            g_printerr ("bus_call :  Warning:  %d %s\n%s\n", Warn->code, Warn->message, debug);
            g_free (debug);
            g_error_free (Warn);
            break;
        }
		case GST_MESSAGE_ELEMENT:
		{
			const GstStructure *s = gst_message_get_structure (msg);
			const gchar *name = gst_structure_get_name (s);
			break;
		}
        default:
		{
            break;
		}
    }
    return TRUE;
}


static GstBusSyncReply create_window (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
	// ignore anything but 'prepare-window-handle' element messages
	if (!gst_is_video_overlay_prepare_window_handle_message (message))
		return GST_BUS_PASS;

	printf("Set Window handle now!");
	//gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message)),   win);

	gst_message_unref (message);

	return GST_BUS_DROP;
}


CGrabber::CGrabber()
{
	_TcamBin = NULL;
	_pipeline = NULL;
	_Properties = NULL;
	_cbfunc = NULL;
	_isLive = false;
	_LastErrorText = "No error.";
	_Overlay = NULL; 
	_bus_watch_id = 0;
 	_OverlayEnabled = false;
	_loop = NULL;
	_PipelineFilename = "";
	_external_callback = nullptr;
	_device = nullptr;
	//LoadPipelineFromFile("facedetect.pipe");
	 
}

CGrabber::~CGrabber()
{
	CloseCamera();
}

/////////////////////////////////////////////////////////////////
// Load a pipeline from text file.
// Parameter:
// PipelineFileName : Name of the txt file or NULL if no file should be used
// Return:
// True, if the pipeline was loaded or created successfully
// False : if the file of passed Filename does not exist.
//
bool CGrabber::LoadPipelineFromFile( const char* PipelineFileName)
{
	if( _PipelineFilename == "") // Default!
	{
		// Display path
		//_PipeLineString = "tee name=t t. ! queue ! videoconvert ! rsvgoverlay name=overlay ! queue ! videoconvert ! videoscale ! capsfilter name=scaler ! clockoverlay ! xvimagesink sync=false name=liveview";
		//_PipeLineString = "tee name=t t. ! queue ! video/x-raw,format=BGRx ! videoconvert ! rsvgoverlay name=overlay ! clockoverlay ! videoscale ! capsfilter name=scaler ! videoconvert ! xvimagesink sync=false name=liveview";
		_PipeLineString = "tee name=t t. ! queue leaky=1 ! video/x-raw,format=BGRx ! videoconvert ! rsvgoverlay name=overlay ! videoconvert ! xvimagesink sync=false name=liveview";
		// Image save branch
		_PipeLineString += " t. ! queue leaky=1 ! appsink name=sink sync=false";
		return true;
	}
	
	struct stat statbuf;

	if (stat(PipelineFileName, &statbuf) != -1) 
	{
		_PipeLineString = "";
		std::ifstream ifs( PipelineFileName );
  		std::string content( (std::istreambuf_iterator<char>(ifs) ),
        		               (std::istreambuf_iterator<char>()    ) );

		_PipeLineString = content;			
		return true;				   

		if (FILE *file = fopen(PipelineFileName, "r")) 
		{
			char buffer;
			int n = 0;

			while( buffer != EOF)
			{	
				buffer= fgetc(file);
				if( buffer > 30)
				{
					printf("%c",buffer);
					_PipeLineString += buffer;
					n++;
				}
			}
			printf("%s\n",_PipeLineString.c_str());
			fclose(file);
			return true;
		}
	}
	else
	{
		_LastErrorText = "Passed pipeline file does not exist.";
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////
//
bool CGrabber::createTcamBin()
{
	if( _TcamBin == NULL )
	{
		_TcamBin = gst_element_factory_make("tcambin", "tcambin");
	}

	if( _TcamBin == NULL )
	{
		printf("Can not instantiate tcambin\n");
		_LastErrorText = "Can not instantiate tcambin.";
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////
// Open camera by GstDevice 
bool CGrabber::OpenCamera(GstDevice *device )
{
	_device = device; // Device should have a recount of 1 here.
	if( ((GObject *) _device)->ref_count == 0)
		printf("%sWARN: device refcount is 0 in open camera. should be at least 1!%s\n",KYEL,KNRM);

	if( ((GObject *) _device)->ref_count > 1)
		printf("%sWARN: _device refcount %d is greater than 1 in open camera. should be at least 1!%s\n",KYEL,((GObject *) _device)->ref_count, KNRM);

	GstStructure* struc = gst_device_get_properties(device);  
	if( struc == nullptr)
	{
		_LastErrorText = "Could not retrieve device properties (gst_device_get_properties())";
		return false;
	}
	_SerialNumber = gst_structure_get_string(struc, "serial");
	_SerialNumber += "-";
	_SerialNumber += gst_structure_get_string(struc, "type");
	_DeviceName = gst_structure_get_string(struc, "model");
	gst_structure_free(struc);
	printf("Open camera by device sn: %s %s\n",_DeviceName.c_str(), _SerialNumber.c_str());

	if(!createTcamBin())
		return false;

	g_object_set(_TcamBin,"tcam-device",_device,NULL);
	gst_element_set_state(_TcamBin, GST_STATE_READY);
	if( gst_element_get_state( _TcamBin, NULL, NULL, 4000000000ll) != GST_STATE_CHANGE_SUCCESS )
	{
		_LastErrorText = "Could not set tcambin to READY.";
		return false;
	}

	if( isDevValid() )
		printf("%sOK: device is valid%s\n",KGRN, KNRM);
	else
		printf("%sWARN: device is valid%s\n",KYEL, KNRM);
	return isDevValid();
}

//////////////////////////////////////////////////////////
//
bool CGrabber::isDevValid()
{
	_LastErrorText = "Device is invalid, no device selected.";
	if( _TcamBin == nullptr)
		return false;

	gchar *testserial = nullptr;
	gchar *testtype = nullptr;
	g_object_get(_TcamBin,"serial",&testserial,"type", &testtype,NULL);

	if( testserial == nullptr)
		return false;

	if( testtype == nullptr)
		return false;

	std::string tmps = testserial;
	tmps += "-";
	tmps += testtype;

	delete testserial;
	delete testtype;


	if( _SerialNumber != tmps)
		return false;

	_LastErrorText = "";
	return true;
}


//////////////////////////////////////////////////
// Cleanup, stop live, delete tcambin and so on.
void CGrabber::CloseCamera()
{
	stopLive();

	if( _TcamBin != NULL )
	{
		gst_element_set_state( _TcamBin, GST_STATE_NULL);
		gst_object_unref(_TcamBin);
		_TcamBin = NULL;
	}
	_isLive = false;
	
	if( _device != nullptr)
	{
		if(((GObject *) _device)->ref_count > 0)
			gst_object_unref(_device);
	}
}

//////////////////////////////////////////////////////////
// Enumerate video formats from tcamsrc in current tcambin.
//
bool CGrabber::EnumerateVideoFormats()
{
	_VideoFormats.enumerate(gettcambinchild("tcambin-source"));
	printf("Grabber Enumvideoformats, %d found\n", _VideoFormats.size() );

	return (_VideoFormats.size() > 0 );
}

/////////////////////////////////////////////////////////////////
// Build the pipeline and start the stream.
bool CGrabber::startLive()
{
	GError *error = NULL;
	if( !isDevValid())
	{
		_LastErrorText ="No device selected.";
		return false;
	}

	if( !LoadPipelineFromFile(""))
		return false;

	printf("%s\n",_PipeLineString.c_str());

 	_pipeline = gst_parse_launch (_PipeLineString.c_str(), &error);
	
	if (error)
	{
		printf( "Error building Camera pipeline: %s\n", error->message);
		_LastErrorText = std::string( "Error building pipeline: ") +  std::string(error->message);
		return false;
	}


	if( !setCaps() )					// Set video format and frame rate
	{
		stopLive();
		return false;
	}

	if( !link_tcambin(true) ) 	// Add our tcambin source
		return false;

	setWindowHandle_int();
	setWindowSize(640,480);

	GstAppSink *appsink = (GstAppSink*)gst_bin_get_by_name(GST_BIN(_pipeline), "sink");
	if( appsink != NULL )
	{
		gst_app_sink_set_max_buffers(appsink, 5);
		gst_app_sink_set_drop(appsink, true); 
        gst_app_sink_set_emit_signals(appsink,true);
		
		GstAppSinkCallbacks callbacks = {};
		callbacks.new_sample = this->AppsinkCallback;
		
		gst_app_sink_set_callbacks( appsink, &callbacks, (gpointer) this,NULL);
		
		gst_object_unref(appsink);
	}
	else
		printf("No appsink found.\n");
    
	// Query the rsvgoverlay, called "Overlay", so we can draw later in it.
	_Overlay = gst_bin_get_by_name(GST_BIN(_pipeline), "overlay");
	if( _Overlay == NULL )
		printf("Overlay not found in the pipeline\n");

	//  add a message handler 
    _bus = gst_pipeline_get_bus (GST_PIPELINE (_pipeline));
    _bus_watch_id = gst_bus_add_watch (_bus, bus_call, _loop);

    gst_object_unref (_bus);

	GValue state2 = G_VALUE_INIT;
	g_value_init(&state2, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(_TcamBin), "tcam-properties-json", &state2);

	printf("\n************* STARTING\n%s\n", g_value_get_string(&state2));

	if( WaitForChangeState(GST_STATE_PLAYING ) )
	{
		GValue state2 = G_VALUE_INIT;
		g_value_init(&state2, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(_TcamBin), "tcam-properties-json", &state2);

		printf("\n************* PLAYING\n%s\n", g_value_get_string(&state2));

		_isLive = true;
		queryChildElements();
		return true;
	}

	stopLive();

	return false;
}

void CGrabber::set_new_frame_callback(std::function<GstFlowReturn(GstAppSink *appsink, gpointer data)>callback, gpointer data)
{
    _external_callback = callback;
    _external_callback_data = data;
}

GstFlowReturn CGrabber::AppsinkCallback(GstAppSink *appsink, gpointer user_data)
{
	CGrabber *pGrabber = static_cast<CGrabber *>(user_data);

	if( pGrabber->_snapImage && pGrabber->_pQImageBuffer != nullptr )
	{
		GstSample* sample = NULL;
		/* Retrieve the buffer */
		g_signal_emit_by_name(appsink, "pull-sample", &sample, NULL);    

		GstMapInfo map;
		GstCaps *caps = gst_sample_get_caps(sample);
		GError *err = NULL;

		GstStructure *structure = gst_caps_get_structure(caps, 0);
		const int width = g_value_get_int(gst_structure_get_value(structure, "width"));
		const int height = g_value_get_int(gst_structure_get_value(structure, "height"));

		GstCaps * capsTo = gst_caps_new_simple("video/x-raw",
											"format", G_TYPE_STRING, "RGB",
											"width", G_TYPE_INT, width,
											"height", G_TYPE_INT, height,
											NULL);


		GstSample *convertedSample = gst_video_convert_sample(sample,capsTo,GST_SECOND,&err);


		if (convertedSample == nullptr) 
		{
			//qWarning() << "gst_video_convert_sample Failed:" << err->message;
		}
		else 
		{
			//qDebug() << "Converted sample caps:" << convertedSample->caps()->toString();

			GstBuffer * buffer = gst_sample_get_buffer(convertedSample);
			gst_buffer_map(buffer, &map, GST_MAP_READ);

			*(pGrabber->_pQImageBuffer) = QImage((const uchar *)map.data,
							width,
							height,
							QImage::Format_RGB888);

			gst_sample_unref(convertedSample); 								
		}
		if(sample)
		{
			gst_sample_unref(sample); 
		}    
		pGrabber->_snapImage = false;
		
	}
	if( pGrabber->_external_callback != nullptr)
	{
		pGrabber->_external_callback(appsink, pGrabber->_external_callback_data);
	}

	return GST_FLOW_OK;
}

int CGrabber::setWindowHandle_int()
{
	GstElement *liveview = gst_bin_get_by_name(GST_BIN(_pipeline), "liveview");
	if( liveview == NULL )
	{
		printf("No live sink found.\n");
		_LastErrorText = "No live sink found.";
		return 0;
	}
	else
	{
		gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (liveview), _WindowHandle);
		gst_object_unref(liveview);
	}
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Helper function for GStreamer state changes with error handling.
//
int CGrabber::WaitForChangeState(GstState newState)
{
	char ErrorText[1024];
	ErrorText[0] ='\0';
	
	GstStateChangeReturn Result = gst_element_set_state( _pipeline, newState);
	if( Result == GST_STATE_CHANGE_FAILURE )
	{
		sprintf(ErrorText,"FAILURE set state() () change to state %d failed\n", newState);
		_LastErrorText = ErrorText;
		return 0;
	}

	Result = gst_element_get_state( _pipeline, NULL, NULL, 4000000000ll);

	if( Result == GST_STATE_CHANGE_FAILURE )
	{
		sprintf(ErrorText,"FAILURE WaitForChangeState() change to state %d failed\n", newState);
		_LastErrorText = ErrorText;
		return 0;
	}

	if( Result == GST_STATE_CHANGE_ASYNC )
	{
		sprintf(ErrorText,"ASYNC WaitForChangeState() change to state %d failed\n", newState);
		_LastErrorText = ErrorText;
		return 0;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// Add or remove the tcambin source from the pipeline at tee element. 
//
bool CGrabber::link_tcambin(bool link)
{
	bool result = true;
	_LastErrorText = "";
	GstElement *teeFilter = gst_bin_get_by_name(GST_BIN(_pipeline), "t");
	//GstElement *CapsFilter = gst_bin_get_by_name(GST_BIN(_pipeline), "capsfilter"); 
	if(teeFilter == NULL)
	{
		_LastErrorText = "Link elements: teeFilter not found!";
		return false;
	}

	if( link )
	{
		if( gst_bin_add(GST_BIN(_pipeline), _TcamBin) )
		{
			if(!gst_element_link( _TcamBin, teeFilter ) )
			{
				_LastErrorText = "Faild to link tcambin to tee.";
				result = false;
			}
		}
		else
		{
			_LastErrorText = "Faild to add tcambin to pipeline.";
			result = false;
		}
	}
	else
	{		
		gst_element_unlink( _TcamBin, teeFilter );

		// gst_bin_add does not addref, but gst_bin_remove does unref.
		// In order to avoid _TcamBin pointing to a dead object, we need to addref
		gst_object_ref( _TcamBin );

		gst_bin_remove(GST_BIN(_pipeline), _TcamBin);
	}
	gst_object_unref(teeFilter);

	return result;
}

/////////////////////////////////////////////////////////////////
// Set the video format caps
//
bool CGrabber::setCaps()
{
	if( _pipeline != NULL )
	{
		if( _VideoFormat.Width() > 0 && _VideoFormat.Height() > 0)
		{
			GstCaps* caps = gst_caps_new_empty();

    		GstStructure* structure = gst_structure_from_string(_VideoFormat.Type(), NULL);

		 	gst_structure_set(structure, 
							"format", G_TYPE_STRING,_VideoFormat.Format(), 
							"width", G_TYPE_INT,_VideoFormat.Width(),
							"height", G_TYPE_INT,_VideoFormat.Height(),
							"framerate", GST_TYPE_FRACTION, (unsigned int )_FrameRate._numerator, _FrameRate._denominator,
							NULL); 

			if(_VideoFormat.isBinning() )						
			{	
				gst_structure_set(structure, "binning", G_TYPE_STRING,_VideoFormat.Binning(),NULL);
				//gst_caps_append(caps,gst_caps_new_simple(_VideoFormat.Type(), "binning", G_TYPE_STRING,_VideoFormat.Binning(),NULL));
			}
			else
			{
				if(_VideoFormat.isSkipping() )						
				{				
					gst_structure_set(structure, "skipping", G_TYPE_STRING,_VideoFormat.Skipping(),NULL) ;
					//gst_caps_append(caps,gst_caps_new_simple(_VideoFormat.Type(), "skipping", G_TYPE_STRING,_VideoFormat.Skipping(),NULL) );
				}
			}
			gst_caps_append_structure(caps, structure);	
			printf("%s\n", gst_caps_to_string(caps));

			g_object_set( _TcamBin, "device-caps",gst_caps_to_string(caps),NULL);
			return true;
		}	
		else
		{
			_LastErrorText = "Invalid video format.";
		}
	}
	else
	{
		_LastErrorText = "setCaps() : Invalid video format";
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
//
bool CGrabber::restartLive()
{
	if( _pipeline != NULL )
	{
		if( _isLive )
			WaitForChangeState(GST_STATE_READY);
		_isLive = false;
		
		if( setCaps() )
		{
			setWindowHandle_int();

			if( WaitForChangeState(GST_STATE_PLAYING ) )
			{
				_isLive = true;
				return true;
			}
			else
				_isLive = false;
			
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
// 
void CGrabber::stopLive()
{
	if( _pipeline != NULL )
	{
		printf("Stoplive\n");
		WaitForChangeState(GST_STATE_READY);

		if( _bus_watch_id > 0)
		{
			g_source_remove(_bus_watch_id);
		}
		link_tcambin(false); // remove our tcambin source

		WaitForChangeState(GST_STATE_NULL );
		gst_object_unref (_pipeline);

		_pipeline = NULL;
	}
	_isLive = false;
}

//////////////////////////////////////////////////////////////////////////
//
void* CGrabber::thread_function(void *data)
{
	GMainLoop* ploop = (GMainLoop*) data;

	//g_main_loop_run(ploop);

	return NULL;
}

void CGrabber::setWindowSize(int width, int height)
{
	if( _pipeline == NULL)
		return;
	
	//if( WaitForChangeState(GST_STATE_READY) )
	{
		GstElement *videoscale = gst_bin_get_by_name(GST_BIN(_pipeline), "scaler");
		if( videoscale != NULL )
		{
			GstCaps* caps; 
			caps = gst_caps_new_simple("video/x-raw", 
										"width", G_TYPE_INT,width,
										"height", G_TYPE_INT,height,
										NULL); 

			
			g_object_set(videoscale, "caps", caps, NULL);
			gst_object_unref(videoscale);
			gst_caps_unref(caps);
		}
		else
		{
			printf("*** scaler element not found\n");
		}
	}
	// WaitForChangeState(GST_STATE_PLAYING);
}

int CGrabber::getImageWidth()
{
	return _VideoFormat.Width();
}

int CGrabber::getImageHeight()
{
	return _VideoFormat.Height();
}

bool CGrabber::isPropertyAvailable( const char *Property )
{
	GError* err = NULL;
    TcamPropertyBase* property_base = tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
                                                                               Property,
                                                                               &err);

    if (err)
    {
        printf("%sINFO: %s while retrieving property: %s%s\n",KBLU,this->getDeviceName().c_str(), err->message, KNRM);
        g_error_free(err);
        err = NULL;
		return false;
	}
	return true;
	//return ( tcam_prop_get_tcam_property((TcamProp *)_TcamBin, (gchar*)Property, &value, &min,&max,&def,&step,&type,&flags,&category,&group) );
}

bool  CGrabber::getPropertyRange(const char* Property, gint64* Min, gint64* Max)
{
	if(_TcamBin != NULL )
	{
		GError* err = NULL;
		TcamPropertyInteger* property = (TcamPropertyInteger*)tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
																				Property,
																				&err);

		if (err)
		{
			printf("%sINFO: %s while retrieving property: %s %s%s\n",KBLU,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
			return false;
		}

		gint64 step;
		tcam_property_integer_get_range(property, Min, Max, &step, &err);

		if (err)
		{
			printf("%s ERROR: %s while retrieving property: %s %s%s\n",KRED,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
			return false;
		}
		return true;

	}
	return false;

}

bool CGrabber::getPropertyValueInt(const char* Property, gint64* Value)
{
	if(_TcamBin != NULL )
	{
		GError* err = NULL;
		TcamPropertyInteger* property = (TcamPropertyInteger*)tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
																				Property,
																				&err);
		if (err)
		{
			printf("%sINFO: %s while retrieving property: %s %s%s\n",KBLU,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
			return false;
		}

		*Value = tcam_property_integer_get_value(property, &err);

		if (err)
		{
			printf("%s ERROR: %s while retrieving property: %s %s%s\n",KRED,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
			return false;
		}
		return true;

	}
	return false;
}

void CGrabber::set_property_int(const char *Property, const gint64 Value )
{
	if(_TcamBin != NULL )
	{
		GError* err = NULL;
		TcamPropertyInteger* property = (TcamPropertyInteger*)tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
																				Property,
																				&err);

		if (err)
		{
			printf("%sINFO: %s while retrieving property: %s %s%s\n",KBLU,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
		}

		tcam_property_integer_set_value(property, Value, &err);

		if (err)
		{
			printf("%s ERROR: %s while retrieving property: %s %s%s\n",KRED,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
		}

		updateOverlay();
	}
	else
	{
		printf("Error: pipeline not built! Call start_camera() first.\n");
	}
}

void CGrabber::set_property_double(const char *Property, const double Value )
{
	GValue val = {};
	char PropertyType[20];
	/*
	if(_TcamBin != NULL )
	{
		strcpy( PropertyType, tcam_prop_get_tcam_property_type((TcamProp *)_TcamBin, (gchar*)Property));
		
		if( strcmp( PropertyType, "double") == 0 )
		{
			g_value_init(&val, G_TYPE_DOUBLE);
			g_value_set_double(&val, Value);
		}
		
		if( !tcam_prop_set_tcam_property((TcamProp *)_TcamBin,(char*)Property , &val) )
		{
			printf("Failed to set %s\n", Property);
		}
		updateOverlay();
	}
	else
	{
		printf("Error: pipeline not built! Call start_camera() first.\n");
	}*/
}

//////////////////////////////////////////////////////////////////////////////////////
//
std::string CGrabber::getEnumProperty(const char *Property)
{
	return "";
}


void CGrabber::setEnumProperty(const char *Property, const char* value)
{
	if( _TcamBin != NULL )
	{
		GError* err = NULL;
		TcamPropertyEnumeration* property_cmd = (TcamPropertyEnumeration*)tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
																				Property,
																				&err);

		if (err)
		{
			printf("%sINFO: %s while retrieving property: %s %s%s\n",KBLU,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
		}

		tcam_property_enumeration_set_value(property_cmd, value, &err);

		if (err)
		{
			printf("%s ERROR: %s while retrieving property: %s %s%s\n",KRED,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
		}
	}
	else
	{
		printf("Error: pipeline not built! Call start_camera() first.\n");
	}
}

GSList* CGrabber::getEnumPropertyValues(const char *Property)
{
	return nullptr;
	//GSList* Values = tcam_prop_get_tcam_menu_entries((TcamProp *)_TcamBin,Property);
	//return Values;
}


void CGrabber::set_property_boolean(const char *Property, const bool  Value )
{
	GValue val = {};
	char PropertyType[20];
	
	if( _TcamBin != NULL )
	{
		/*
		strcpy( PropertyType, tcam_prop_get_tcam_property_type((TcamProp *)_TcamBin, (gchar*)Property));
		
		if( strcmp( PropertyType, "integer") == 0 )
		{
			g_value_init(&val, G_TYPE_INT);
			g_value_set_int(&val, Value);
		}
		else
		{
			if( strcmp( PropertyType, "boolean") == 0 )
			{
				g_value_init(&val, G_TYPE_BOOLEAN);
				if( Value == 1 )
					g_value_set_boolean(&val, true);
				else
					g_value_set_boolean(&val, false);
			}
			else
			{
				return;
			}
		}	
		
		if( !tcam_prop_set_tcam_property((TcamProp *)_TcamBin ,(char*)Property , &val) )
		{
			printf("Failed to set %s\n", Property);
		}
		updateOverlay();
	*/
	}
	else
	{
		printf("Error: pipeline not built! Call start_camera() first.\n");
	}
}

bool CGrabber::get_property_boolean(const char *Property, bool *Value )
{
	GValue value = {};
	GValue min = {};
	GValue max = {};
	GValue def = {};
	GValue step = {};
	GValue type = {};
	GValue flags = {};
	GValue category = {};
	GValue group = {};

	char PropertyType[20];
	
	if( _TcamBin != NULL )
	{
		/*
		strcpy( PropertyType, tcam_prop_get_tcam_property_type((TcamProp *)_TcamBin, (gchar*)Property));
		
		if( tcam_prop_get_tcam_property((TcamProp *)_TcamBin, (gchar*)Property, &value, &min,&max,&def,&step,&type,&flags,&category,&group))
		{
			if( strcmp( PropertyType, "boolean") == 0 )
			{
				*Value = g_value_get_boolean(&value);
				return true;
			}
		}*/
	}
	return false;
}

void CGrabber::set_property_button(const char *Property )
{
	if( _TcamBin != NULL )
	{
		GError* err = NULL;
		TcamPropertyCommand* property_cmd = (TcamPropertyCommand*)tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
																				Property,
																				&err);

		if (err)
		{
			printf("%sINFO: %s while retrieving property: %s %s%s\n",KBLU,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
		}

		tcam_property_command_set_command(property_cmd,&err);

		if (err)
		{
			printf("%s ERROR: %s while retrieving property: %s %s%s\n",KRED,this->getDeviceName().c_str(), Property, err->message, KNRM);
			g_error_free(err);
			err = NULL;
		}
	}
	else
	{
		printf("Error: pipeline not built! Call start_camera() first.\n");
	}
}



void CGrabber::set_property_camera_enum(const char *Property, const char* Value )
{
	if( _TcamBin != NULL )
	{
		updateOverlay();
	}
	else
	{
		printf("Error: pipeline not built! Call start_camera() first.\n");
	}
}

bool CGrabber::getTriggerMode()
{
	bool bValue = false; 
	if( get_property_boolean("Trigger Mode", &bValue) )
		return bValue;
	else
	{
		std::string mode = getEnumProperty( "Trigger Mode" );
		if( mode == "On")
			return true;
	}
	return false;
}


void CGrabber::enableOverlay( bool bEnable)
{
	_OverlayEnabled = bEnable;
	updateOverlay();
}

/////////////////////////////////////////////////////////////
// Draw the rectangle of Exposure Auto ROI on the video.
//
void CGrabber::updateOverlay()
{
	char svgdata[1024];
	gint64 roi[4];
	
	if(_Overlay == NULL) 
		return;
	
	if(_OverlayEnabled)
	{
		bool propertyfound = true;
		char tmp[1024];
		strcpy(svgdata, "<svg>");

		if( getPropertyValueInt("AutoFunctionsROILeft", &roi[0]) )  // Aravis / GigE 33
		{
			getPropertyValueInt("AutoFunctionsROIWidth", &roi[1]);
			getPropertyValueInt("AutoFunctionsROITop", &roi[2]);
			getPropertyValueInt("AutoFunctionsROIHeight", &roi[3]);
		}
		else
		{ 
			propertyfound = false;
		}		
		
		if( propertyfound)		
		{
			sprintf( tmp,"<rect x='%ld' y='%ld' width='%ld' height='%ld' stroke='red' stroke-width='2px' fill='none'/>",
				roi[0]+1, roi[2]+1, roi[1]-2, roi[3]-2);
			strcat(svgdata,tmp);
		}

		// focus

		gint64 min;
		gint64 max;
		gint64 step;
		GError *err = nullptr;

		TcamPropertyInteger *autofocusroiwidth =  (TcamPropertyInteger*)tcam_property_provider_get_tcam_property(TCAM_PROPERTY_PROVIDER(_TcamBin),
																				"AutoFocusROIWidth",
																				&err);
		if( err )			
		{
			g_error_free(err);
			err = NULL;
		}
		else
		{																				


			if( getPropertyValueInt("AutoFocusROILeft", &roi[0]) )
			{
				getPropertyValueInt("AutoFocusROIWidth", &roi[1]);
				getPropertyValueInt("AutoFocusROITop", &roi[2]);
				getPropertyValueInt("AutoFocusROIHeight", &roi[3]);

				// Try to check, whether we have software or hardware auto focus ROI
				tcam_property_integer_get_range(autofocusroiwidth, &min, &max, &step, &err);
				if( max != _VideoFormat.Width())
				{
					gint64 p = 0;
					if( getPropertyValueInt("SensorWidth",&p))
					{
						getPropertyValueInt("OffsetX",&p);
						roi[0] -= p;
						getPropertyValueInt("OffsetY",&p);
						roi[2] -= p;
					}
				}				
				sprintf( tmp,"<rect x='%ld' y='%ld' width='%ld' height='%ld' stroke='blue' stroke-width='2px' fill='none'/>",
					roi[0]+1, roi[2]+1, roi[1]-2, roi[3]-2);
				strcat(svgdata,tmp);

			}
		}
		strcat(svgdata, "</svg>");

		g_object_set(_Overlay,"data", svgdata, NULL);
	}
	else
		g_object_set(_Overlay,"data", "", NULL);
}


std::vector<std::string> CGrabber::enumCameras()
{
	std::vector<std::string> connectedCameras;
	
	TcamProp *tcam = (TcamProp*)gst_element_factory_make("tcamsrc", "tcam");

	GstDeviceMonitor* monitor = gst_device_monitor_new();
	// We are only interested in devices that are in the categories
	// Video and Source and tcam
	gst_device_monitor_add_filter(monitor, "Video/Source/tcam", NULL);

	GList* devices = gst_device_monitor_get_devices(monitor);

	for (GList* elem = devices; elem; elem = elem->next)
	{
		GstDevice* device = (GstDevice*) elem->data;
		GstStructure* struc = gst_device_get_properties(device);

		std::string tmpSerial = gst_structure_get_string(struc, "serial");
		tmpSerial += "-";
		tmpSerial += gst_structure_get_string(struc, "type");
		std::transform(tmpSerial.begin(), tmpSerial.end(), tmpSerial.begin(), ::tolower);
		connectedCameras.push_back(tmpSerial);

		gst_structure_free(struc);
		gst_object_unref(device);
	}

	g_list_free(devices);
	gst_object_unref(monitor);

	return connectedCameras;
}

///////////////////////////////////////////////////////////////////////
// Get the GstDevice, which has the serianumber passed by searchserial
// The returned GstDevice must be unreferenced by the caller.
//
GstDevice *CGrabber::getDeviceBySerial( std::string searchserial)
{
	GstDevice *retdevice = nullptr;

	TcamProp *tcam = (TcamProp*)gst_element_factory_make("tcamsrc", "tcam");

	GstDeviceMonitor* monitor = gst_device_monitor_new();
	// We are only interested in devices that are in the categories
	// Video and Source and tcam
	gst_device_monitor_add_filter(monitor, "Video/Source/tcam", NULL);

	GList* devices = gst_device_monitor_get_devices(monitor);

	for (GList* elem = devices; elem; elem = elem->next)
	{
		GstDevice* device = (GstDevice*) elem->data;
		GstStructure* struc = gst_device_get_properties(device);

		std::string tmpSerial = gst_structure_get_string(struc, "serial");
		tmpSerial += "-";
		tmpSerial += gst_structure_get_string(struc, "type");

		gst_structure_free(struc);
		std::transform(tmpSerial.begin(), tmpSerial.end(), tmpSerial.begin(), ::tolower);
		if( tmpSerial == searchserial)
			retdevice = device;
		else
			gst_object_unref(device);
	}

	g_list_free(devices);
	gst_object_unref(monitor);

	return retdevice; 

}

/////////////////////////////////////////////////////////////
// Select device dialog
//
bool CGrabber::showDeviceSelectionDlg()
{
	CDeviceSelectionDlg cDlg(NULL);
	if( cDlg.exec() == 1 )
	{

		//OpenCamera(cDlg.getSerialNumber());
		OpenCamera(cDlg.getDevice() );
		EnumerateVideoFormats(); // Warum?: Weil der Grabber eine eigene Videoformatliste hat. 

		_VideoFormat = cDlg.getVideoFormat(); //*GetSelectedItemData<CVideoFormat*>(cboVideoFormats);
   		_FrameRate =  cDlg.getFrameRate();

		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////
// Select device dialog
//
bool CGrabber::showPropertyDlg()
{
	CPropertiesDialog Properties(nullptr);
	
	// Pass the tcambin element to the properties dialog
	// so in knows, which device do handle
	Properties.SetCamera(this);
	Properties.exec();		
	return true;
}


bool CGrabber::saveDeviceStatetoFile( const char* filename)
{
	return true;
	FILE *pFile = fopen(filename,"w");
	if( pFile != NULL && _TcamBin != NULL )
	{
		gchar *state = NULL;
		g_object_get(_TcamBin,"state",&state,NULL);
		if( state != NULL)
		{
			fprintf(pFile, "%s\n", state);
			g_free(state);
		}
		fclose(pFile);
		return true;
	}
	return false;
}

bool CGrabber::loadDeviceStateFromFile( const  char* filename)
{
	std::ifstream t(filename);
	std::string state((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());	


	if( state.length() > 0)
	{
		g_object_set(_TcamBin,"state",state.c_str(),NULL);
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////
// Complete device saving, not properites only. 
void CGrabber::saveDevice(std::string FileName)
{
	Json::StyledStreamWriter jsonstream;
    std::ofstream fout2(FileName);
    jsonstream.write(fout2, getJson() );
    fout2.close(); 
}

void CGrabber::loadDevice(std::string FileName)
{
	try
	{
		std::ifstream ifs(FileName);
        Json::Reader reader;
		Json::Value camera;
		reader.parse(ifs, camera); 
		setJson(camera);
	}
	catch(std::ifstream::failure ex)
	{
		std::cout << ex.what() << std::endl;
	}
}


#include <iostream>
#include <sstream>
Json::Value CGrabber::getJson()
{
	Json::Value cam;

	cam["serial"] = getSerialNumber();
	cam["videoformat"] = _VideoFormat.getJson();
	cam["framerate"] = _FrameRate.getJson();
	
	GValue state = G_VALUE_INIT;
    g_value_init(&state, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(_TcamBin), "tcam-properties-json", &state);

	std::stringstream is;
	is << g_value_get_string(&state);
	Json::Value jstate;
	JSONCPP_STRING errs;
	Json::CharReaderBuilder builder;
	bool ok = Json::parseFromStream(builder, is , &jstate, &errs);
 	
	cam["state"] = g_value_get_string(&state);
	g_value_unset(&state);
	return cam;
}

/////////////////////////////////////////////////////////////////
// Apply the properties json string to tcambin. 
void CGrabber::setPropertiesJson(Json::Value &cam)
{
	GValue state = G_VALUE_INIT;
	g_value_init(&state, G_TYPE_STRING);


	printf("\n************* LOADED\n%s\n", cam["state"].asString().c_str());
	g_value_set_string(&state, cam["state"].asString().c_str());
	g_object_set_property(G_OBJECT(_TcamBin), "tcam-properties-json", &state);
	GValue state2 = G_VALUE_INIT;
	g_value_init(&state2, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(_TcamBin), "tcam-properties-json", &state2);
	printf("\n************* FROM DEV\n%s\n", g_value_get_string(&state2));





	gint64 v = 0;
	                        
	if(getPropertyValueInt("AutoFocusROIHeight", &v ))
	{
		printf("Test: %ld\n", v);
	}
	else
	{
		printf("Test: %s\n", _LastErrorText.c_str());
	}

	g_value_unset(&state);
}

bool CGrabber::setJson(Json::Value &cam)
{
	GstDevice* dev = getDeviceBySerial( cam["serial"].asString());
	if( dev == nullptr)
		return false;

	if( OpenCamera(dev))  // Adds a reference
	{
		if( EnumerateVideoFormats() )
		{
			_VideoFormat.setJson( cam["videoformat"]);
			_FrameRate.setJson(cam["framerate"]);

			setPropertiesJson(cam);
			gst_object_unref(dev);

			return true;
		}
		else
		{
			printf("%sSetJson: NO Videoformats!%s\n",KRED,KNRM);
		}

	}
	gst_object_unref(dev);
	return false;
}

///////////////////////////////////////////////////////////////////////////
// Query the child elements in the tcambin, in case we use tcambin
// Save them in a list for later usage.
//
void CGrabber::queryChildElements()
{
    //_tcambinchilds.clear();

    if (GST_IS_BIN(_TcamBin))
    {
		printf("Loaded child modules:\n");
        GstElement *child;
        GValue value = {};
        g_value_init (&value, GST_TYPE_ELEMENT);

        GstIterator *iterator = gst_bin_iterate_elements (GST_BIN( _TcamBin));
        while (gst_iterator_next (iterator, &value) == GST_ITERATOR_OK)
        {
            child = (GstElement*)g_value_get_object (&value);
            std::string childname = gst_element_get_name(child);
            //_tcambinchilds.push_back(childname);
            printf("\t%s\n",childname.c_str());
			if( childname == "tcambin-source")
				g_object_set(child,"do-timestamp",true,NULL);

            g_value_reset (&value);
        }
        g_value_unset (&value);
        gst_iterator_free (iterator);
    }
    //gst_object_unref(tcambin);
}       

//////////////////////////////////////////////////////////////////
// Get tcamsrc from current tcambin
// Increases refcount by 1. Callfunction must call g_object_unref()
//
GstElement *CGrabber::gettcambinchild(std::string childname)
{
	GstElement *returnElement = nullptr;

    if (GST_IS_BIN(_TcamBin))
    {
        GstElement *child;
        GValue value = {};
        g_value_init (&value, GST_TYPE_ELEMENT);

        GstIterator *iterator = gst_bin_iterate_elements (GST_BIN( _TcamBin));
        while (gst_iterator_next (iterator, &value) == GST_ITERATOR_OK)
        {
            child = (GstElement*)g_value_get_object (&value);
            std::string currentchildname = gst_element_get_name(child);
            
			if( childname == currentchildname)
			{
				returnElement = (GstElement*)gst_object_ref(child);
			}
            g_value_reset (&value);
        }
        g_value_unset (&value);
        gst_iterator_free (iterator);
    }

	return returnElement;
}


bool CGrabber::snapImage(QImage &qimage, int timeout)
{
	_pQImageBuffer = &qimage;
	_snapImage = true;
	usleep(timeout * 1000);
	_snapImage = false;
	//qimage = _QImageBuffer;
	return true;
}


//////////////////////////////////////////////////////////////////
// Draw an overly. data contains an xml string defining the graphics
// primitives.
void CGrabber::SetOverlayGraphicString( const char* data)
{
    if( _Overlay == NULL )
        return;
    
    g_object_set(_Overlay,"data",data,NULL);
}