#ifndef CPROPERTYCRTL_H
#define CPROPERTYCRTL_H

#include <QMainWindow>
#include <QtWidgets>
#include <QLabel>
#include <QAction>
#include <QtGui>

#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>

#include <vector>
#include <QTabWidget>
#include <tcam-property-1.0.h>

class CPropertyControl : public QWidget
{
	Q_OBJECT
	
	public:
		int Property;

		CPropertyControl(TcamPropertyBase *property, QDialog *pParent)
		{
			_Updating = false;
			_baseproperty = property;
			_pParent = pParent;
		}

		~CPropertyControl()
		{
		}
    
    	virtual void set_value(gint64 value)
		{
			printf("%s function set_value not implemented\n",_name.c_str() );
		};

		void updateControlsLocked();
		void enableToolTips(bool on);
		virtual void UpdateValue();
			
	protected:
		std::string _name;
		bool _Updating;
		QDialog *_pParent;
		std::list<QWidget*> _widgets;
	private:
		TcamPropertyBase *_baseproperty;

};

class CPropertyCtrl_int : public CPropertyControl
{
	Q_OBJECT

	public: 
		CPropertyCtrl_int( TcamPropertyBase *property, QGridLayout *pLayout, QDialog *pParent);
		void set_value(gint64 value);
		void UpdateValue();

		
	private:
		TcamPropertyInteger *_property;
		QLabel *Label;
		QSlider *Slider;
		QLabel *Value;
		QHBoxLayout *hl;
		gint64 _step;
		
		int get_value(int iPos);
		void show_value( int i);
		std::vector<uint> scale;
		
		void exposure_scale( gint64 i_x_min,  gint64 i_x_max, const int num_samples, const double s=8.0);
		
	private slots:
		void OnMovedSlider(int value);
		
};


class CPropertyCtrl_float : public CPropertyControl
{
	Q_OBJECT

	public: 
		CPropertyCtrl_float( TcamPropertyBase *property,  QGridLayout *pLayout,QDialog *pParent);
		void set_value(gdouble value);
		void UpdateValue();
	private:
		TcamPropertyFloat *_property;
		QLabel *Label;
		QSlider *Slider;
		QLabel *Value;
		QHBoxLayout *hl;
		std::vector<double> dscale;

		gdouble  get_value(int iPos);
		void show_value( gdouble i);
		
		void double_scale( gdouble x_min,  gdouble x_max, const int num_samples );
		void exposure_scale(const gdouble x_min, const gdouble x_max, const int num_samples=256, const gdouble s=8.0);
		
	private slots:
		void OnMovedSlider(int value);

};

class CPropertyCtrl_boolean : public CPropertyControl
{
	Q_OBJECT

	public: 
		CPropertyCtrl_boolean(TcamPropertyBase* property, QGridLayout *pLayout,QDialog *pParent);
		void set_value(gboolean value);
		void UpdateValue();
		
	private:
		TcamPropertyBoolean *_property;
		QCheckBox *Check;

	private slots:
		void OnClickedCheckkBox(int); 
};

class CPropertyCtrl_command : public CPropertyControl
{
	Q_OBJECT

	public: 
		CPropertyCtrl_command(TcamPropertyBase* property, QGridLayout *pLayout,QDialog *pParent);
		void set_value(int value);
		void UpdateValue(){};
		
	private:
		TcamPropertyCommand *_property;
		QPushButton *Push;

	private slots:
		void OnPushButton();
	
};


class CPropertyCtrl_enum : public CPropertyControl
{
	Q_OBJECT

	public: 
		CPropertyCtrl_enum( TcamPropertyBase* property, QGridLayout *pLayout,QDialog *pParent);
		void set_value(const char* value);
		void UpdateValue();

	private:
		TcamPropertyEnumeration *_property;
		QLabel *Label;
		QComboBox *Combobox;

	private slots:
		void OnCBOIndexChanged(int);
};



#endif