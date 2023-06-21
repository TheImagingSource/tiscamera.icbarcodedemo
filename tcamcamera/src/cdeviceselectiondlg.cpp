/*
 * Copyright  The Imaging Source Europe GmbH
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


#include "cdeviceselectiondlg.h"
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <qdialog.h>
#include <glib.h>
#include <gst/gst.h>
#include <cctype>
#include "customformatdlg.h"

/////////////////////////////////////////////////////////////////////
// Helper templates
//
template<typename T>
void DeleteItems(QComboBox *CBO)
{
	
	for( int i = 0; i<CBO->count(); i++ )
	{
		T *pFR = (T*)CBO->itemData(i).value<void*>();
		delete (pFR);
	}
	
	CBO->clear();
}

template<typename T>
T GetSelectedItemData(QComboBox *CBO)
{
	return (T)CBO->itemData(CBO->currentIndex()).value<void*>();
}



/**********************************************************************************
	In the constructor of the dialog the available cameras and devices are enumerated
	and listed in to combo boxes.
*/	
CDeviceSelectionDlg::CDeviceSelectionDlg(QWidget* parent ):QDialog(parent)
{
	CreateUI();
    _monitor = gst_device_monitor_new();
     gst_device_monitor_add_filter(_monitor, "Video/Source/tcam", NULL);

	EnumerateDevices();
}

CDeviceSelectionDlg::~CDeviceSelectionDlg()
{
	ClearComboBoxes();
    gst_object_unref(_monitor);
}

void CDeviceSelectionDlg::EnumerateDevices()
{

  	guint major;
    guint minor;
    guint micro;
    guint nano;

	gst_version(&major, &minor, &micro,&nano);

    //https://gstreamer.freedesktop.org/documentation/gstreamer/gstdevicemonitor.html?gi-language=c
    GList* devices = gst_device_monitor_get_devices(_monitor);

    for (GList* elem = devices; elem; elem = elem->next)
    {
        GstDevice* device = (GstDevice*) elem->data;
        GstStructure* devstruc = gst_device_get_properties( device );

        std::string devstring = gst_structure_get_string(devstruc, "model");
        devstring += " ";
        devstring += gst_structure_get_string(devstruc, "serial");
        devstring += " ";
        devstring += gst_structure_get_string(devstruc, "type");

        QVariant v;
		v.setValue((void*)device);

        cboCameras->addItem(devstring.c_str(), v);

        gst_structure_free(devstruc);
    }

    g_list_free(devices);
    cboCameras->setCurrentIndex(0);
}

void CDeviceSelectionDlg::CreateUI()
{
	this->setWindowTitle("Device Selection");
	
	_MainLayout  = new QVBoxLayout();
	setLayout( _MainLayout );

	QGridLayout *GridLayout = new QGridLayout();

	lblAction = new QLabel;
	lblAction->setAlignment(Qt::AlignCenter);
	
	QLabel *lblCam = new QLabel("Camera: ");
    cboCameras = new QComboBox();
	cboCameras->setMinimumWidth(250);
	QObject::connect(cboCameras, SIGNAL(currentIndexChanged(int)), this, SLOT(OncboCamerasChanced(int)));

	GridLayout->addWidget(lblCam,0,0);
	GridLayout->addWidget(cboCameras,0,1);
	
	QLabel *lblFormat = new QLabel("Format: ");
	
    cboVideoFormats = new QComboBox();
    QObject::connect(cboVideoFormats, SIGNAL(currentIndexChanged(int)), this, SLOT(OncboVideoFormatsChanced(int)));

	_CustomizeButton = new QPushButton(tr("Customize"));
    connect(_CustomizeButton, SIGNAL(released()), this, SLOT(OnCustomizeButton()));
	_CustomizeButton->setEnabled(false);

	GridLayout->addWidget(lblFormat,1,0);
	GridLayout->addWidget(cboVideoFormats,1,1);
	GridLayout->addWidget(_CustomizeButton,1,2);

	QLabel *lblfps = new QLabel("Frame Rate: ");
	cboFrameRates = new QComboBox();
	GridLayout->addWidget(lblfps,2,0);	
	GridLayout->addWidget(cboFrameRates,2,1);	

	QLabel *lblPipeline = new QLabel("Pipeline: ");
	QLabel *lblPipeName = new QLabel("Standard");
	QPushButton	*ButtonPipeline = new QPushButton(tr("Browse"));
    connect(ButtonPipeline, SIGNAL(released()), this, SLOT(OnBrowsePipeline()));

	GridLayout->addWidget(lblPipeline,3,0);	
	GridLayout->addWidget(lblPipeName,3,1);	
	GridLayout->addWidget(ButtonPipeline,3,2);	

	_MainLayout->addLayout(GridLayout);

	_MainLayout->addWidget(lblAction);

	QHBoxLayout *buttons = new QHBoxLayout();

	//////////////////////////////////////////////////////////
	UpdateButton = new QPushButton(tr("Update"));
    connect(UpdateButton, SIGNAL(released()), this, SLOT(OnUpdateButton()));
    buttons->addWidget(UpdateButton);

	
	CancelButton = new QPushButton(tr("Cancel"));
    connect(CancelButton, SIGNAL(released()), this, SLOT(OnCancel()));
    buttons->addWidget(CancelButton);

	OKButton = new QPushButton(tr("OK"));
    connect(OKButton, SIGNAL(released()), this, SLOT(OnOK()));
    buttons->addWidget(OKButton);

	_MainLayout->addLayout(buttons);
}

/*******************************************************
	TODO: Implement something useful here.
*/
void CDeviceSelectionDlg::OnUpdateButton()
{
	lblAction->setText("Searching devices");
	lblAction->repaint();
	UpdateButton->setEnabled(false);
	OKButton->setEnabled(false);
	CancelButton->setEnabled(false);
	this->setCursor(Qt::WaitCursor);
	
	ClearComboBoxes();
	EnumerateDevices();

	lblAction->setText("");

	UpdateButton->setEnabled(true);
	OKButton->setEnabled(true);
	CancelButton->setEnabled(true);

	this->setCursor(Qt::ArrowCursor);
}

void CDeviceSelectionDlg::OnOK()
{
	printf("OnOK\n");
	
	GstDevice* device = (GstDevice*) GetSelectedItemData<DeviceDesc*>(cboCameras);  
	CVideoFormat *pVideoFormat = GetSelectedItemData<CVideoFormat*>(cboVideoFormats);
	CFrameRate *pFrameRate =  GetSelectedItemData<CFrameRate*>(cboFrameRates);
	this->done(Accepted);
}



void CDeviceSelectionDlg::OnCancel()
{
	strcpy(_CapsString,"");
	this->done(Rejected);
}

void CDeviceSelectionDlg::ClearComboBoxes()
{
	
	DeleteItems<CFrameRate>(cboFrameRates);
	DeleteItems<CVideoFormat>(cboVideoFormats);

	for( int i = 0; i<cboCameras->count(); i++ )
	{
		GstDevice *pFR = (GstDevice *)cboCameras->itemData(i).value<void*>();
		gst_object_unref(pFR);
	}
	
	cboCameras->clear();
}

/******************************************************
	 New camera was selected
*/
void CDeviceSelectionDlg::OncboCamerasChanced(int Index)
{
	DeleteItems<CFrameRate>(cboFrameRates);
	DeleteItems<CVideoFormat>(cboVideoFormats);

	if( Index == -1 ) 
		return;

	EnumerateVideoFormats();
	
	for( int i = 0; i < _VideoFormats.size(); i++ )
	{
		CVideoFormat *vf = new CVideoFormat(_VideoFormats[i]);

		QVariant v;
		v.setValue((void*)vf );
		cboVideoFormats->addItem(vf->ToString().c_str(),v);
	}
    cboVideoFormats->setCurrentIndex(0);

	_CustomizeButton->setEnabled(
		_VideoFormats.isRangeType((GstDevice*) GetSelectedItemData<DeviceDesc*>(cboCameras))
		);

	lblAction->setText("");

	return;
} 


////////////////////////////////////////////////////////////////////
//
bool CDeviceSelectionDlg::EnumerateVideoFormats()
{
	lblAction->setText("Query video formats.");
	lblAction->repaint();
	_VideoFormats.enumerate((GstDevice*) GetSelectedItemData<DeviceDesc*>(cboCameras));
	lblAction->setText("");
	lblAction->repaint();
	return true;
}


////////////////////////////////////////////////////////////////////
//
void CDeviceSelectionDlg::OncboVideoFormatsChanced(int Index)
{

	if( Index == -1 ) 
		return;

	DeleteItems<CFrameRate>(cboFrameRates);

	CVideoFormat *pVF = GetSelectedItemData<CVideoFormat*>(cboVideoFormats);
	
	for( int i = 0; i < pVF->_framerates.size();i++ )
	{
		CFrameRate *pFR = new CFrameRate( pVF->_framerates.at(i));
		QVariant v;
		v.setValue((void*)pFR);

		cboFrameRates->addItem(pFR->ToString().c_str(),v);
	}
	
} 

////////////////////////////////////////////////////////////////////////
//
void CDeviceSelectionDlg::OnCustomizeButton()
{
	CVideoFormat *pVF = GetSelectedItemData<CVideoFormat*>(cboVideoFormats);
	//CustomFormatDlg::FORMATDESC_t f;

    //f = {240,4,1920,1080,8,4,640,480,0,0};


    CustomFormatDlg dlg( GetSelectedItemData<CVideoFormat*>(cboVideoFormats), this);

	if( dlg.exec() == QDialog::Accepted)
    {
		CVideoFormat *vf = new CVideoFormat(dlg.getVideoFormat());

		QVariant v;
		v.setValue((void*)vf );
		cboVideoFormats->addItem(vf->ToString().c_str(),v);
		cboVideoFormats->setCurrentIndex(cboVideoFormats->count()-1);
	}
}

void CDeviceSelectionDlg::OnBrowsePipeline()
{

}
//////////////////////////////////////////////////////////////////////
// Get selected item datas
//
GstDevice* CDeviceSelectionDlg::getDevice()
{
	return (GstDevice*) GetSelectedItemData<DeviceDesc*>(cboCameras);  
}

CVideoFormat CDeviceSelectionDlg::getVideoFormat()
{
	CVideoFormat *pVF = GetSelectedItemData<CVideoFormat*>(cboVideoFormats);
	return *pVF;
}

CFrameRate CDeviceSelectionDlg::getFrameRate()
{
	CFrameRate *pFrameRate =  GetSelectedItemData<CFrameRate*>(cboFrameRates);
	return *pFrameRate;
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////







