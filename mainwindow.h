#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QtGui>
#include <ic_barcode.h>

#include "cgrabber.h"


// Define custom event identifier for new images received
const QEvent::Type NEW_IMAGE_EVENT = static_cast<QEvent::Type>(QEvent::User + 1);

// Define custom event subclass for new image
class NewImageEvent : public QEvent
{
    public:
        NewImageEvent(const int customData1, const int customData2):
            QEvent(NEW_IMAGE_EVENT),
            m_customData1(customData1),
            m_customData2(customData2)
        {
        }

        int getCustomData1() const
        {
            return m_customData1;
        }

        int getCustomData2() const
        {
            return m_customData2;
        }

    private:
        int m_customData1;
        int m_customData2;
};

// Define your custom event identifier
const QEvent::Type BARCODE_FOUND_EVENT = static_cast<QEvent::Type>(QEvent::User + 1);

// Define your custom event subclass
class BarcodeFoundEvent : public QEvent
{
    public:
        BarcodeFoundEvent(const int customData1, const int customData2):
            QEvent(BARCODE_FOUND_EVENT),
            m_customData1(customData1),
            m_customData2(customData2)
        {
        }

        int getCustomData1() const
        {
            return m_customData1;
        }

        int getCustomData2() const
        {
            return m_customData2;
        }

    private:
        int m_customData1;
        int m_customData2;
};


QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void postNewImageEvent(const int customData1, const int customData2);
    void postBarcodeFoundEvent(const int customData1, const int customData2);

private slots:
    void on_actionSave_Image_triggered();

private:
    void OnSelectDevice();
    void OnDeviceProperties();
    void LoadLastUsedDevice();

private slots:

    void on_actionLoad_Configuration_triggered();
    void on_actionSave_Configuration_triggered();
    void on_actionExit_triggered();

    void on_actionSelect_triggered();
    void on_actionProperties_triggered();


private:
    Ui::MainWindow* _ui = nullptr; 
    CGrabber _Grabber;

    static GstFlowReturn NewImageCallback(GstAppSink *appsink, gpointer user_data);
    struct callback_user_data
    {
        MainWindow *pMainWindow;
        int framecount;
        ICBarcode_Scanner* pIC_BarcodeScanner;
        int FoundBarcodes;
        std::vector<ICBarcode_Result> BarCodes;
        GstSample *sample;
        bool busy;
    } _callback_user_data = {};
    void customEvent(QEvent * event);
    void onNewImageEvent(const NewImageEvent *event);
    void handleBarcodeFoundEvent(const BarcodeFoundEvent *event);

    std::string getDeviceFile();
    void startVideo();
    static void findBarcodes(callback_user_data *pData);
};

#endif // MAINWINDOW_H
