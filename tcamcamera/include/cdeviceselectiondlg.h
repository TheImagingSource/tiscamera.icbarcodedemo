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

#ifndef CDEVICESELECTIONDLG_H
#define CDEVICESELECTIONDLG_H


#include <QtWidgets>
#include "videoformat.h"
#include "videoformats.h"

/////////////////////////////////////////////////////////////////////


class CDeviceSelectionDlg : public QDialog
{
    Q_OBJECT

	public:
		CDeviceSelectionDlg(QWidget* parent );
		~CDeviceSelectionDlg();
	
		char* getCapString(){return _CapsString;};
	//	GstDevice* getDevice(){ return _SelectedDevice.device;};
		/*	
		std::string getSerialNumber() {return _SelectedDevice.SerialNumber;};
		int getWidth() {return _SelectedDevice.Width;};
		int getHeight() {return _SelectedDevice.Height;};
		int getFPSNominator() {return _SelectedDevice.fpsNominator;};
		int getFPSDeNominator() {return _SelectedDevice.fpsDeNominator;};
		*/
		GstDevice* getDevice();
		CVideoFormat getVideoFormat();
		CFrameRate getFrameRate();
		
	private slots:
		void OnCancel();
		void OnOK();
		void OnUpdateButton();
		void OncboCamerasChanced(int Index);
		void OncboVideoFormatsChanced(int Index);
		void OnCustomizeButton();
		void OnBrowsePipeline();


	private:
		void ClearComboBoxes();
		void CreateUI();
		char _CapsString[2048];
		char _SerialNumber[2048];
		void DeleteFrameRatesCBO();
		void DeleteVideoFormatsCBO();
		void EnumerateDevices();
		bool EnumerateVideoFormats();

        GstDeviceMonitor* _monitor;

		struct DeviceDesc
		{
			GstDevice *device;
			std::string SerialNumber;
			std::string Backend;
			std::string Name;
			std::string Format;
			int Width;
			int Height;
			int fpsNominator;
			int fpsDeNominator;
		};

		DeviceDesc _SelectedDevice;
		std::vector<DeviceDesc> _Devices;
		//std::vector <CVideoFormat> ;
		CVideoFormats _VideoFormats;
		
		
		QVBoxLayout *_MainLayout;
		QPushButton* _CustomizeButton;
		QPushButton *CancelButton;
		QPushButton *OKButton;
		QPushButton *UpdateButton;
		QLabel *lblCameraName;
		QComboBox *cboCameras;
		QComboBox *cboVideoFormats;
		QComboBox *cboFrameRates;
		QLabel *lblAction;
};

#endif // CDEVICESELECTIONDLG_H
