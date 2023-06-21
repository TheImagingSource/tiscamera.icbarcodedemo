#include "propertyctrl.h"

#include <stdio.h>
#include <stdlib.h>
#include "cpropertiesdialog.h"


///////////////////////////////////////////////////////
// Query the current property value and show it on the UI
void CPropertyControl::UpdateValue()
{
}

void CPropertyControl::updateControlsLocked()
{
	GError *err = nullptr;
	gboolean locked = tcam_property_base_is_locked(_baseproperty,&err);
	gboolean available = tcam_property_base_is_available(_baseproperty,&err);

	for(auto w : _widgets)
	{
		if(w != nullptr)
		{
			if( tcam_property_base_get_access(_baseproperty) == TCAM_PROPERTY_ACCESS_RO)	
				w->setEnabled(true);
			else	
			{
				if( available)
					w->setEnabled(!locked);
				else
					w->setEnabled(false);
			}
		}
	}
}

void CPropertyControl::enableToolTips(bool on)
{
	for(auto w : _widgets)
	{
		if (w != NULL)
		{
			if(on)
				w->setToolTip( tcam_property_base_get_description(_baseproperty));
			else
				w->setToolTip("");
		}
	}
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

CPropertyCtrl_int::CPropertyCtrl_int(TcamPropertyBase *property, QGridLayout *pLayout, QDialog *pParent) : CPropertyControl(property, pParent)
{
	_property = TCAM_PROPERTY_INTEGER(property);
	Label = new QLabel( tcam_property_base_get_display_name(property));
	if(  strlen( tcam_property_base_get_display_name(property) )== 0)
		Label->setText(QString(tcam_property_base_get_name(property)) + " !");

	Slider = nullptr;
	Value = new QLabel();
	GError* err = NULL;
	
	if( tcam_property_base_get_access(property) != TCAM_PROPERTY_ACCESS_RO)		
	{
		Slider = new QSlider(Qt::Horizontal);

		gint64 min;
		gint64 max;
		
		tcam_property_integer_get_range(_property, &min, &max, &_step, &err);

		if( tcam_property_integer_get_representation(_property) == TCAM_PROPERTY_INTREPRESENTATION_LOGARITHMIC )
		{
			Slider->setMinimum( 0);
			Slider->setMaximum(99);
			exposure_scale(min, max, 100);
		}
		else
		{
			Slider->setMinimum( min / _step );
			Slider->setMaximum( max / _step);
			//printf("%s %d\n", tcam_property_base_get_display_name(property), step);
			//Slider->setTickInterval(step);
			//Slider->setSingleStep(step);
			//Slider->setPageStep(step*10);
		}
		QObject::connect(Slider, SIGNAL(sliderMoved(int)), SLOT(OnMovedSlider(int)));
		QObject::connect(Slider, SIGNAL(valueChanged(int)), SLOT(OnMovedSlider(int)));
	}
	gint64 value = tcam_property_integer_get_value(_property, &err);
	set_value(value);

	int LayoutRows = pLayout->rowCount();
	pLayout->addWidget(Label,LayoutRows,0);
	_widgets.push_back(Label);
	if( Slider != nullptr)
	{
		pLayout->addWidget( Slider,LayoutRows,1);
		_widgets.push_back(Slider);
		pLayout->addWidget(Value,LayoutRows,2);
	}
	else
		pLayout->addWidget(Value,LayoutRows,1);

	_widgets.push_back(Value);


}

void CPropertyCtrl_int::set_value(gint64 value)
{
	if( tcam_property_base_get_access((TcamPropertyBase*)_property) != TCAM_PROPERTY_ACCESS_RO)
	{
		if( scale.size() > 0  )
		{
			for( int i = 0; i < scale.size() -1; i++ )
			{
				if( value >= scale[i] && value <= scale[i+1] )
				{
					printf("slider : %d \t%ld",i, value);
					Slider->setValue(i);
					show_value(scale[i]);
				}
			}
		}
		else
		{
			Slider->setValue(value / _step);
			show_value(value);
		}
	}
	else
	{
		show_value(value);
	}	
}

int CPropertyCtrl_int::get_value(int iPos)
{
	if(  scale.size() > 0  )
	{
		return scale[iPos];
	}
	return iPos * _step;
}

void CPropertyCtrl_int::show_value( int i)
{
	char str[20];
	if( tcam_property_integer_get_unit(_property) != nullptr)
		sprintf(str,"%d %s",i, tcam_property_integer_get_unit(_property));
	else
		sprintf(str,"%d",i);
		
	if( Value != NULL )
	{
		Value->blockSignals(true);
		Value->setAlignment(Qt::AlignRight|Qt::AlignCenter);
		Value->setText(str);
		Value->blockSignals(false);
	}
}

void CPropertyCtrl_int::OnMovedSlider(int iPos)
{
	if(_Updating)
		return;
//	int p = (iPos+1) / Slider->tickInterval();
// 	p = p * Slider->tickInterval();
    GError* err = NULL;
	tcam_property_integer_set_value(_property, get_value(iPos * _step),&err);
	show_value( get_value(iPos));
	((CPropertiesDialog*)_pParent)->UpdateControls();
}

void CPropertyCtrl_int::exposure_scale( gint64 i_x_min,  gint64 i_x_max, const int num_samples, const double s)
{
	double x_min = (double) i_x_min;
	double x_max = (double) i_x_max;

	double rangelen = log(x_max) - log(x_min);

	for( int i = 0; i<num_samples - 1; i++)
	{
		double val = exp(log(x_min) + rangelen / num_samples * i);

		if( val < x_min) val=x_min;
		if( val > x_max) val=x_max;
        scale.push_back((uint)val);
	}
	scale.push_back((uint)x_max);

}

void CPropertyCtrl_int::UpdateValue()
{
	_Updating = true;
	if(Slider != nullptr)
		Slider->blockSignals(true);
	GError *err = nullptr;
	gint64 value = tcam_property_integer_get_value(_property, &err);
	set_value(value);
	if(Slider != nullptr)
		Slider->blockSignals(false);
	_Updating = false;

}


///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

CPropertyCtrl_float::CPropertyCtrl_float(TcamPropertyBase *property , QGridLayout *pLayout, QDialog *pParent) : CPropertyControl(property, pParent)
{
	GError* err = NULL;
	_property = TCAM_PROPERTY_FLOAT(property);

	Label = new QLabel(  tcam_property_base_get_display_name(property));
	if(  strlen( tcam_property_base_get_display_name(property) )== 0)
		Label->setText(QString(tcam_property_base_get_name(property)) + " !");
	Slider = nullptr;
	Value = new QLabel();

	if( tcam_property_base_get_access(property) != TCAM_PROPERTY_ACCESS_RO)		
	{
		Slider = new QSlider(Qt::Horizontal);

		Slider->setMinimum( 0);
		Slider->setMaximum(99);

		gdouble min;
		gdouble max;
		gdouble step;
		tcam_property_float_get_range(_property, &min, &max, &step, &err);

		if( tcam_property_float_get_representation(_property) == TCAM_PROPERTY_FLOATREPRESENTATION_LOGARITHMIC)
			exposure_scale(min , max, 100);
		else
			double_scale(min, max, 100);

		QObject::connect(Slider, SIGNAL(sliderMoved(int)), SLOT(OnMovedSlider(int)));
		QObject::connect(Slider, SIGNAL(valueChanged(int)), SLOT(OnMovedSlider(int)));
	}

	int LayoutRows = pLayout->rowCount();
	pLayout->addWidget(Label,LayoutRows,0);
	_widgets.push_back(Label);

	if( Slider != nullptr)
	{
		pLayout->addWidget( Slider,LayoutRows,1);
		_widgets.push_back(Slider);
		pLayout->addWidget( Value,LayoutRows,2);
	}
	else
		pLayout->addWidget( Value,LayoutRows,1);
	_widgets.push_back(Value);

	gdouble value = tcam_property_float_get_value((TcamPropertyFloat*)_property, &err);
	set_value(value);
}

void CPropertyCtrl_float::set_value(gdouble value)
{
	if( tcam_property_base_get_access((TcamPropertyBase*)_property) != TCAM_PROPERTY_ACCESS_RO)
	{
		if( dscale.size() > 0  )
		{
			for( int i = 0; i < dscale.size() -1; i++ )
			{
				if( value >= dscale[i] && value <= dscale[i+1] )
				{
					Slider->setValue(i);
				}
			}
		}
	}
	show_value(value);
}

double CPropertyCtrl_float::get_value(int iPos)
{
	if(  dscale.size() > 0  )
	{
		return dscale[iPos];
	}
	return (double)iPos;
}

void CPropertyCtrl_float::show_value( double i)
{
	char str[50];
	if( tcam_property_float_get_unit(_property) != nullptr)
		sprintf(str,"%6.2f %s",i, tcam_property_float_get_unit(_property));
	else
		sprintf(str,"%6.2f",i);

	if( Value != NULL )
	{
		Value->setAlignment(Qt::AlignRight|Qt::AlignCenter);
		Value->setText(str);
	}
}

void CPropertyCtrl_float::OnMovedSlider(int iPos)
{
	if(_Updating)
		return;

	GError *err = nullptr;

	tcam_property_float_set_value(_property, get_value(iPos),  &err);
	show_value( get_value(iPos));
	((CPropertiesDialog*)_pParent)->UpdateControls();
}

/// Doubles linear.
void CPropertyCtrl_float::double_scale( double x_min,  double x_max, const int num_samples )
{
	double steps = (x_max - x_min) / double(num_samples);

	for( int i = 0; i<num_samples-1; i++)
	{
		double val = x_min + (double)i*steps;

		if( val < x_min) val=x_min;
		if( val > x_max) val=x_max;
        dscale.push_back(val);
	}
    dscale.push_back(x_max);
}

void CPropertyCtrl_float::exposure_scale( double x_min,  double x_max, const int num_samples, const double s)
{
	double rangelen = log(x_max) - log(x_min);

	for( int i = 0; i<num_samples; i++)
	{
		double val = exp(log(x_min) + rangelen / num_samples * i);

		if( val < x_min) val=x_min;
		if( val > x_max) val=x_max;
        dscale.push_back(val);
	}
}

void CPropertyCtrl_float::UpdateValue()
{
	_Updating = true;
	if(Slider != nullptr)
		Slider->blockSignals(true);
	GError *err = nullptr;
	gdouble value = tcam_property_float_get_value(_property, &err);
	set_value(value);
	if(Slider != nullptr)
		Slider->blockSignals(false);
	_Updating = false;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

CPropertyCtrl_boolean::CPropertyCtrl_boolean( TcamPropertyBase* property, QGridLayout *pLayout, QDialog *pParent) : CPropertyControl(property, pParent)
{
	GError *err = nullptr;

	_property = TCAM_PROPERTY_BOOLEAN(property);
	Check = new QCheckBox( tcam_property_base_get_display_name(property) );
	gboolean value = tcam_property_boolean_get_value(_property, &err);
	Check->setCheckState( value ? Qt::Checked : Qt::Unchecked );

	int LayoutRows = pLayout->rowCount();
	pLayout->addWidget(Check,LayoutRows,1);
	_widgets.push_back(Check);
	QObject::connect(Check, SIGNAL(stateChanged(int)), this, SLOT(OnClickedCheckkBox(int)));

}

void CPropertyCtrl_boolean::set_value(gboolean value)
{
	Check->setCheckState( value? Qt::Checked : Qt::Unchecked );
}

void CPropertyCtrl_boolean::OnClickedCheckkBox(int value)
{
	if(_Updating)
		return;
	GError *err = nullptr;
	tcam_property_boolean_set_value(_property,Check->checkState() == Qt::Checked, &err);
	((CPropertiesDialog*)_pParent)->UpdateControls();
}

void CPropertyCtrl_boolean::UpdateValue()
{
	_Updating = true;
	Check->blockSignals(true);
	GError *err = nullptr;
	gboolean value = tcam_property_boolean_get_value(_property, &err);
	Check->setCheckState( value ? Qt::Checked : Qt::Unchecked );
	Check->blockSignals(false);
	_Updating = false;

}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

CPropertyCtrl_command::CPropertyCtrl_command(TcamPropertyBase* property, QGridLayout *pLayout, QDialog *pParent) : CPropertyControl(property, pParent)
{
	_property = TCAM_PROPERTY_COMMAND(property);

	Push = new QPushButton(  tcam_property_base_get_display_name(property));
	int LayoutRows = pLayout->rowCount();
	pLayout->addWidget(Push,LayoutRows,1);
	_widgets.push_back(Push);

	QObject::connect(Push, SIGNAL(released()), this, SLOT(OnPushButton()));
}

void CPropertyCtrl_command::set_value(int value)
{

}

void CPropertyCtrl_command::OnPushButton()
{
	GError *err = nullptr;
	tcam_property_command_set_command(_property,&err);
	((CPropertiesDialog*)_pParent)->UpdateControls();
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

CPropertyCtrl_enum::CPropertyCtrl_enum( TcamPropertyBase* property,  QGridLayout *pLayout, QDialog *pParent) : CPropertyControl(property, pParent)
{
	GError *err = nullptr;
	_property = TCAM_PROPERTY_ENUMERATION(property);

	Label = new QLabel( tcam_property_base_get_display_name(property) );
	if(  strlen( tcam_property_base_get_display_name(property) )== 0)
		Label->setText(QString(tcam_property_base_get_name(property)) + " !");

	Combobox = new QComboBox();

	int LayoutRows = pLayout->rowCount();
	pLayout->addWidget(Label,LayoutRows,0);
	pLayout->addWidget(Combobox,LayoutRows,1);

	_widgets.push_back(Label);
	_widgets.push_back(Combobox);

	const gchar *current = tcam_property_enumeration_get_value(_property, &err);

	GSList* enum_entries = tcam_property_enumeration_get_enum_entries(_property, &err);
	int index = -1;
	while (enum_entries)
	{
		index++;
		Combobox->addItem( (char*) enum_entries->data );
		if( strcmp( current, (char*)enum_entries->data ) == 0)
			Combobox->setCurrentIndex(index);

		enum_entries = enum_entries->next;
	}
	g_slist_free_full(enum_entries, g_free);

	QObject::connect(Combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCBOIndexChanged(int)));
}

void CPropertyCtrl_enum::set_value(const char *value)
{
	// TODO
}

void CPropertyCtrl_enum::OnCBOIndexChanged(int)
{
	if(_Updating)
		return;
	GError *err = nullptr;
	if( _Updating )
		return;
	QByteArray ba = Combobox->itemText(Combobox->currentIndex()).toLatin1();
	std::string Selected = ba.data();

	tcam_property_enumeration_set_value(_property,ba.data(), &err);
	((CPropertiesDialog*)_pParent)->UpdateControls();
}

void CPropertyCtrl_enum::UpdateValue()
{
	_Updating = true;
	Combobox->blockSignals(true);

	GError *err = nullptr;
	const gchar *current = tcam_property_enumeration_get_value(_property, &err);

	for( int i = 0; i < Combobox->count(); i++ )
	{
		QByteArray ba = Combobox->itemText(i).toLatin1();
		if( strcmp( current, (const char*)ba.data()) == 0)
		{
			Combobox->setCurrentIndex(i);
			break;
		}
	}
	Combobox->blockSignals(false);

	_Updating = false;
}


//#include "propertyctrl.moc"
