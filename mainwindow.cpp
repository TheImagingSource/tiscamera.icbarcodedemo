/*
 * Copyright 2015 bvtest <email>
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

#include "mainwindow.h"
#include <iostream>
#include "./ui_mainwindow.h"
#include <stdio.h>
#include <fmt/format.h>
#include <QFileDialog>
#include <QMessageBox>
#include <pwd.h>
#include <QImage>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);
    _ui->statusBar->addPermanentWidget(_ui->lblCamera);


    // Instantiate the barcode scanner.
    _callback_user_data.pIC_BarcodeScanner =  ICBarcode_CreateScanner();
    _callback_user_data.BarCodes.resize(50);

    // Configure the barcode scanner to try harder. If barcodes are hardly detected,
    // pass an 1 instead of a 0.
    ICBarcode_SetParam(_callback_user_data.pIC_BarcodeScanner, IC_BARCODEPARAMS_TRY_HARDER, 1);

    // Enable barcode checksum support. 
	ICBarcode_SetParam(_callback_user_data.pIC_BarcodeScanner, IC_BARCODEPARAMS_CHECK_CHAR, 1);

    // Define. which formats the barcode library shall detect.
	int formats = 0;
	formats |= ICBarcode_Format::IC_BARCODEFORMAT_CODE_128;
	formats |= ICBarcode_Format::IC_BARCODEFORMAT_CODE_93;
	formats |= ICBarcode_Format::IC_BARCODEFORMAT_EAN_13;
	formats |= ICBarcode_Format::IC_BARCODEFORMAT_EAN_8;
	formats |= ICBarcode_Format::IC_BARCODEFORMAT_UPC_A;
	formats |= ICBarcode_Format::IC_BARCODEFORMAT_QR_CODE;
    formats |= ICBarcode_Format::IC_BARCODEFORMAT_DATA_MATRIX;
	//formats |= ICBarcode_Format::IC_BARCODEFORMAT_INTERLEAVED_2_OF_5;
	//formats |= ICBarcode_Format::IC_BARCODEFORMAT_CODE_39;
    
	ICBarcode_SetBarcodeFormats(_callback_user_data.pIC_BarcodeScanner, formats);

    LoadLastUsedDevice();
}


MainWindow::~MainWindow()
{
    delete _ui;
}

// File Menu
void MainWindow::on_actionLoad_Configuration_triggered()
{
    _Grabber.CloseCamera();
    QFileDialog fileDialog(this, tr("Save Configuration"),tr("./"), tr("Json Files (*.json)"));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setDefaultSuffix("json");
    
    if( fileDialog.exec()) 
    {
        _Grabber.loadDevice(fileDialog.selectedFiles().first().toStdString());
        startVideo();
    }
}

void MainWindow::on_actionSave_Configuration_triggered()
{
    if( _Grabber.isDevValid() )
    {
        QFileDialog fileDialog(this, tr("Save Configuration"),tr("./"), tr("Json Files (*.json)"));
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        fileDialog.setDefaultSuffix("json");
        
        if( fileDialog.exec()) 
        {
            _Grabber.saveDevice(fileDialog.selectedFiles().first().toStdString());
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

// Device Menu
void MainWindow::on_actionSelect_triggered()
{
    if( _Grabber.isLive() )
    {
        _Grabber.CloseCamera();
    }
    if( _Grabber.showDeviceSelectionDlg() )
    {
        _Grabber.saveDevice(getDeviceFile());
        startVideo();
    }
}

void MainWindow::on_actionProperties_triggered()
{
    if( _Grabber.isDevValid() )
    {
        _Grabber.showPropertyDlg();
        _Grabber.saveDevice(getDeviceFile());
    }
}

void MainWindow::on_actionSave_Image_triggered()
{
    QImage image = {};
    _ui->statusBar->showMessage("Snapping image");
    if( _Grabber.snapImage(image, 1000) )
    {
        QFileDialog fileDialog(this, tr("Save Iamge"),tr("./"), tr("PNG Files (*.png)"));
        fileDialog.setDefaultSuffix("png");
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        
        if( fileDialog.exec()) 
        {
            image.save(fileDialog.selectedFiles().first());
        }
    }
    _ui->statusBar->showMessage("");
}

//////////////////////////////////////////////////////////////////////////
void MainWindow::startVideo()
{
    if( _Grabber.isDevValid() )
    {
        _ui->lblCamera->setText(_Grabber.getDeviceName().c_str());
        WId xwinid = _ui->VideoWidget->winId();

        _Grabber.SetWindowHandle(xwinid);

        _callback_user_data.framecount = 0;
        _callback_user_data.pMainWindow = this;
        //callback
        _Grabber.set_new_frame_callback(this->NewImageCallback, &_callback_user_data);
        _callback_user_data.busy = false;
       	if( !_Grabber.startLive() )
        {
            std::string msg = _Grabber.getDeviceName() +" "+  _Grabber.getSerialNumber() + "\n"+ _Grabber.GetLastErrorText();
            QMessageBox::critical(this,tr("Error start video"),tr(msg.c_str()),QMessageBox::Ok);
        }
    }
}

std::string MainWindow::getDeviceFile()
{
    std::string devicefile = getpwuid(getuid())->pw_dir;
    devicefile += "/IC Barcode.json";
    return devicefile;
}

void MainWindow::LoadLastUsedDevice()
{
    _Grabber.loadDevice(getDeviceFile());
    startVideo();
}

/////////////////////////////////////////////////////////////////////
// Callback function, called on each new image. Here the barcode 
// finding is performed
GstFlowReturn MainWindow::NewImageCallback(GstAppSink *appsink, gpointer user_data)
{
    if( user_data != nullptr)
    {
        auto data = (callback_user_data*)user_data;
        data->framecount++;
        if( data->pMainWindow != nullptr)
            data->pMainWindow->postNewImageEvent(0,0);

        if(data->busy)
            return GST_FLOW_OK;
        data->busy = true;

        GstSample *sample = gst_app_sink_pull_sample(appsink);
        data->sample = sample;

        findBarcodes(data);
        
        gst_sample_unref(sample);
        data->busy = false;
    }
    return GST_FLOW_OK;
}

/////////////////////////////////////////////////////////////////////////
// This function finds the barcodes and stires the result in the
// callback user data.
void MainWindow::findBarcodes(callback_user_data *data)
{
        GstBuffer *buffer = gst_sample_get_buffer(data->sample);
        GstMapInfo info;

        gst_buffer_map(buffer, &info, GST_MAP_READ);
        
        if (info.data != NULL) 
        {
            int width, height ;
            const GstStructure *str;
            // info.data contains the image data as blob of unsigned char 

            GstCaps *caps = gst_sample_get_caps(data->sample);
            // Get a string containg the pixel format, width and height of the image        
            str = gst_caps_get_structure (caps, 0);    
                // Now query the width and height of the image
            gst_structure_get_int (str, "width", &width);
            gst_structure_get_int (str, "height", &height);

            if( strcmp( gst_structure_get_string (str, "format"),"BGRx") == 0)  
            {
                // If the buffer is a 32 bit RGB32 image, it must be converted to
                // 8 bit grayscale. before barcodes can be searched.

                std::vector<unsigned char> buffer;
                buffer.resize(width * height);    
                ICBarcode_Transform_BGRA32_to_Y800(
                    buffer.data(), info.data,	
                    width, height,
                    width * 4,
                    width
                    );
                data->FoundBarcodes = ICBarcode_FindBarcodes(data->pIC_BarcodeScanner,
                                            &buffer[0],
                                            width,height,width,
                                            data->BarCodes.data(), data->BarCodes.size());
            }
            else
            {
                data->FoundBarcodes = ICBarcode_FindBarcodes(data->pIC_BarcodeScanner,
                                            info.data,
                                            width,height,width,
                                            data->BarCodes.data(), data->BarCodes.size());
            }
            
            //if(pCustomData->FoundBarcodes> 0)
            data->pMainWindow->postBarcodeFoundEvent(1,2);
        }
        
        // Calling Unref is important!
        gst_buffer_unmap (buffer, &info);
}

void MainWindow::customEvent(QEvent * event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the MainWindow's thread. We could ask more events here.

    if(event->type() == NEW_IMAGE_EVENT)
    {
        onNewImageEvent(static_cast<NewImageEvent *>(event));
    }

    if(event->type() == BARCODE_FOUND_EVENT)
    {
        handleBarcodeFoundEvent(static_cast<BarcodeFoundEvent *>(event));
    }

}

//////////////////////////////////////////////////////////////////////////////////////////
// Event handler, so we can call MainWindow functions from the camera thread.
// It is called from the callback function, which runs in Grabber's Gstreamer thread
void MainWindow::postNewImageEvent(const int customData1, const int customData2)
{   
    // This method (postMyCustomEvent) can be called from any thread

    QApplication::postEvent(this, new NewImageEvent(customData1, customData2));   
}
//////////////////////////////////////////////////////////////////////////////////////////
// Event handler, so we can call MainWindow functions from the camera thread.
void MainWindow::postBarcodeFoundEvent(const int customData1, const int customData2)
{   
    // This method (postMyCustomEvent) can be called from any thread
    QApplication::postEvent(this, new BarcodeFoundEvent(customData1, customData2));   
}


//////////////////////////////////////////////////////////////////////////////
// Here we are in the MainWindow thread and can access QWidgets.
void MainWindow::onNewImageEvent(const NewImageEvent *event)
{

    _ui->lblCamera->setText(
         fmt::format("{} : {} frames", _Grabber.getDeviceName(), _callback_user_data.framecount).c_str());
}


///////////////////////////////////////////////////////////////////////////////
// This event is called from the GStreamer callback of TCamcamera class.
// Barcodes are already deceted and they are in the _CustomData.FoundBarcodes
// member.
// The "svg" string will contain the xml string for the rectangles around the
// found barcodes.
//
void MainWindow::handleBarcodeFoundEvent(const BarcodeFoundEvent *event)
{
    std::string BarcodeList = "";
    std::string svg;
    char primitive[1024];
    svg = "<svg>\n";
    if( _callback_user_data.FoundBarcodes == 0 )
        return;

    for( int i = 0 ; i < _callback_user_data.FoundBarcodes; i++)
    {
        BarcodeList += _callback_user_data.BarCodes[i].Text; // Get the contents of a decoded barcode
        BarcodeList += "\n";

        printf("%s\n",_callback_user_data.BarCodes[i].Text);

        // Create a polygon using the x/y coordinates of the corner of a barcode in the image
        svg += "\t<polygon points=\"";
        for(int p = 0; p < _callback_user_data.BarCodes[i].ResultPointsLength; p++)
        {
            if( p > 0)
            {
                svg += ",";
            }
            sprintf(primitive,"%d,%d",
                (int)_callback_user_data.BarCodes[i].ResultPoints[p].x,
                (int)_callback_user_data.BarCodes[i].ResultPoints[p].y);
            svg += primitive;
        }
        svg += "\" fill='none' stroke='red' stroke-width='3px'/>\n";
    }
    svg += "</svg>\n";
    _Grabber.SetOverlayGraphicString(svg.c_str()); // Pass the graphics string to the overly
     _ui->txtFoundBarcodes->setText(BarcodeList.c_str());  // Show the decoded barcodes in the barcode window.
}