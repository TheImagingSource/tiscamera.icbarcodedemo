#ifndef CustomFormatDlg_H
#define CustomFormatDlg_H
    
#include <QDialog>
#include <QMainWindow>
#include <QLabel>
#include <QSlider>
#include "videoformat.h"

class CustomFormatDlg : public QDialog
{
    Q_OBJECT
    
    public:
        struct FORMATDESC_t
        {
            int minwidth ;
            int minheight ;
            int maxwidth ;
            int maxheight ;
            int stepwidth ;
            int stepheight ;
            int currentwidth ;
            int currentheight ;
            int selectedtwidth ;
            int selectedheight ;
        };

        //CustomFormatDlg(QMainWindow *parent = nullptr, int minimumWidth = 0,int minimumHeight = 0,int maximumWidth = 0,int maximumHeight = 0,int actualWidth = 0,int actualHeight = 0);
        CustomFormatDlg(FORMATDESC_t &formatdesc, QWidget *parent = nullptr);
        CustomFormatDlg(CVideoFormat* pStartingFormat, QWidget *parent = nullptr);
        ~CustomFormatDlg();
        int getWidth();
        int getHeight();
        CVideoFormat getVideoFormat();

        FORMATDESC_t _formatdesc;

    private slots:
        void Slider1Changed(int value);
        void Slider2Changed(int value);
        void onOK();
        void onCancel();


    private: 
        QLabel *labelForActualValueSlider1;
        QLabel *labelForActualValueSlider2;
        QLabel *rect;
        QSlider *sliderHeight;
        QSlider *sliderWidth;
        CVideoFormat _customFormat;
        //QMainWindow *main;
        void Repaint();
        void CreateUI();

    signals:
        
};
    
    
#endif // CustomFormatDlg_H