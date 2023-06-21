#include "customformatdlg.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QPixmap>

CustomFormatDlg::CustomFormatDlg(CVideoFormat* pStartingFormat, QWidget *parent) : QDialog(parent)
{
    _formatdesc.currentwidth = pStartingFormat->Width();
    _formatdesc.currentheight = pStartingFormat->Height();
    _formatdesc.maxwidth = pStartingFormat->Width();
    _formatdesc.maxheight = pStartingFormat->Height();
    _formatdesc.minwidth = pStartingFormat->WidthMin();
    _formatdesc.minheight = pStartingFormat->HeightMin();
    _formatdesc.stepwidth = pStartingFormat->WidthStep();
    _formatdesc.stepheight = pStartingFormat->HeightStep();

    _customFormat = *pStartingFormat;
    
    CreateUI();
}


CustomFormatDlg::CustomFormatDlg(FORMATDESC_t &formatdesc, QWidget *parent) : QDialog(parent)
{
    CustomFormatDlg::_formatdesc = formatdesc;
    CreateUI();
}

void CustomFormatDlg::CreateUI()
{
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *MainLayout = new QVBoxLayout();
    QGridLayout *GridLayout = new QGridLayout();
    QGridLayout *GridLayoutButtons = new QGridLayout();
    
    //---------------------------------------------------------------------------------------------------
    //Height Slider
    sliderHeight = new QSlider(Qt::Vertical);
    QGridLayout *HeightSliderGridLayout = new QGridLayout();
    sliderHeight->setMinimum(1);

    double sliderMaxHeight = (double)_formatdesc.maxheight / _formatdesc.stepheight;
    sliderHeight->setMaximum(sliderMaxHeight);

    double sliderValHeight = (double)_formatdesc.currentheight / _formatdesc.stepheight;
    sliderHeight->setValue(sliderValHeight);
    //Minimum Label
    QLabel *minimumValueSlider1 = new QLabel(sliderHeight);
    QString minSlid1Val = QString::number(_formatdesc.minheight);
    minimumValueSlider1->setText(minSlid1Val);
    
    //Maximum Label
    QLabel *maximumValueSlider1 = new QLabel(sliderHeight);
    QString maxSlid1Val = QString::number(_formatdesc.maxheight);
    maximumValueSlider1->setText(maxSlid1Val);
    
    //Actual Value Label
    labelForActualValueSlider1 = new QLabel();
    QString actualSlid1Val = QString::number(_formatdesc.currentheight);
    labelForActualValueSlider1->setText(actualSlid1Val);

    //Add Slider Height to Layout
    HeightSliderGridLayout->addWidget( maximumValueSlider1,0,0 );
    HeightSliderGridLayout->addWidget( sliderHeight,1,0 );
    HeightSliderGridLayout->addWidget( minimumValueSlider1,2,0 ); 
    HeightSliderGridLayout->addWidget( labelForActualValueSlider1,1,1);

    GridLayout->addLayout(HeightSliderGridLayout,0,1);
    
    //---------------------------------------------------------------------------------------------------

    //Width Slider
    QGridLayout *WidthSliderGridLayout = new QGridLayout();
    QVBoxLayout *VerticalSliderLayout = new QVBoxLayout();
    sliderWidth = new QSlider(Qt::Horizontal);
    sliderWidth->setMinimum(1);

    double sliderMaxWidth = (double)_formatdesc.maxwidth / _formatdesc.stepwidth;
    sliderWidth->setMaximum((int)sliderMaxWidth);
    double sliderValWidth = (double)_formatdesc.currentwidth / _formatdesc.stepwidth;
    sliderWidth->setValue((int)sliderValWidth);

     //Minimum Label
    QLabel *minimumValueSlider2 = new QLabel(sliderWidth);
    QString minSlid2Val = QString::number((_formatdesc.minwidth));
    minimumValueSlider2->setText(minSlid2Val);
    
    //Maximum Label
    QLabel *maximumValueSlider2 = new QLabel(sliderWidth);
    QString maxSlid2Val = QString::number(_formatdesc.maxwidth);
    maximumValueSlider2->setText(maxSlid2Val);
    
    //Actual Value Label
    labelForActualValueSlider2 = new QLabel();
    QString actualSlid2Val = QString::number(_formatdesc.currentwidth);
    labelForActualValueSlider2->setText(actualSlid2Val);

    
    WidthSliderGridLayout->addWidget( minimumValueSlider2,0,0);
    
    VerticalSliderLayout->addWidget( sliderWidth);
    VerticalSliderLayout->addWidget(labelForActualValueSlider2);
    WidthSliderGridLayout->addLayout(VerticalSliderLayout,0,1);
    WidthSliderGridLayout->addWidget( maximumValueSlider2,0,2);
    
    GridLayout->addLayout(WidthSliderGridLayout,1,0);

    //---------------------------------------------------------------------------------------------------
    // Draw Rectangle

    QHBoxLayout *RectangleLayout = new QHBoxLayout();
    rect = new QLabel();
    //Repaint();
    RectangleLayout->addWidget(rect);
    GridLayout->addLayout(RectangleLayout,0,0 );

    //---------------------------------------------------------------------------------------------------
    // Buttons
    QPushButton *btnOK = new QPushButton("OK");
    QPushButton *btnCancel = new QPushButton("Cancel");
    GridLayout->addWidget( btnOK,2,0);
    GridLayout->addWidget( btnCancel,2,1);
    //---------------------------------------------------------------------------------------------------


    MainLayout->addLayout(GridLayout);
    MainLayout->addLayout(GridLayoutButtons);
    setLayout(MainLayout);


    //EventHandler
    
    QObject::connect(sliderHeight,SIGNAL(valueChanged(int)),this,SLOT(Slider1Changed(int)));
    QObject::connect(sliderWidth,SIGNAL(valueChanged(int)),this,SLOT(Slider2Changed(int)));
    connect(btnOK, SIGNAL(released()), this, SLOT(onOK()));
    connect(btnCancel, SIGNAL(released()), this, SLOT(onCancel()));
}

CustomFormatDlg::~CustomFormatDlg()
{

}

//Height
void CustomFormatDlg::Slider1Changed(int value)
{
    //Do something here
    int stepSize = _formatdesc.stepheight;
    int newValue = stepSize * value;
    QString qstringValue = QString::number(newValue);
    labelForActualValueSlider1->setText(qstringValue);
    Repaint();
}

//Width
void CustomFormatDlg::Slider2Changed(int value)
{
    //Do something here
    int stepSize = _formatdesc.stepwidth;
    int newValue = stepSize * value;
    QString qstringValue = QString::number(newValue);
    labelForActualValueSlider2->setText(qstringValue);
    Repaint();
}

void CustomFormatDlg::Repaint()
{
    int pixmapWidth = sliderWidth->maximum();
    int pixmapHeight = sliderHeight->maximum();
    QPixmap *pix = new QPixmap(pixmapWidth,pixmapHeight);
    QPainter *paint = new QPainter(pix);
    paint->setPen(*(new QColor(255,34,255,255)));

    

    int paintWidth = (int)sliderWidth->value()-1;
    int paintHeight = (int)sliderHeight->value()-1;


    paint->drawRect((pixmapWidth - paintWidth) / 2 ,(pixmapHeight- paintHeight ) / 2 , paintWidth, paintHeight);
    rect->setPixmap(*pix);
}


void CustomFormatDlg::onOK()
{
    _customFormat.setWidth( sliderWidth->value() * _formatdesc.stepwidth);
    _customFormat.setHeigth( sliderHeight->value() * _formatdesc.stepheight) ;

    this->done(Accepted);
}

void CustomFormatDlg::onCancel()
{
    this->done(Rejected);
}

CVideoFormat CustomFormatDlg::getVideoFormat()
{
    return _customFormat;
}

int CustomFormatDlg::getWidth()
{
    int val = (int)(sliderWidth->value() * _formatdesc.stepwidth);
    return val;
}

int CustomFormatDlg::getHeight()
{
    int val = (int)(sliderHeight->value() * _formatdesc.stepheight);
    return val;
}
