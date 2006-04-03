/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999  Klaus-Dieter Möller
*               2000, 2002 kd.moeller@t-online.de
*               
* This file is part of the KDE Project.
* KmPlot is part of the KDE-EDU Project.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
*/

// KDE includes
#include <dcopclient.h>
#include <kapplication.h>
#include <kglobal.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>

// local includes
#include "xparser.h"
#include <QList>

#include <assert.h>


XParser * XParser::m_self = 0;

XParser * XParser::self( bool * modified )
{
	if ( !m_self )
	{
		assert( modified );
		m_self = new XParser( *modified );
	}
	
	return m_self;
}


XParser::XParser(bool &mo) : DCOPObject("Parser"), m_modified(mo)
{
        // setup slider support
	setDecimalSymbol( KGlobal::locale()->decimalSymbol() );
}

XParser::~XParser()
{
}

bool XParser::getext( Ufkt *item, const QString fstr )
{
  	bool errflg = false;
   	int p1, p2, p3, pe;
	QString tstr;
	pe = fstr.length();
	if ( fstr.indexOf( 'N' ) != -1 )
		item->f0.visible = false;
	else
	{
		if ( fstr.indexOf( "A1" ) != -1 )
			item->f1.visible = true;
		if ( fstr.indexOf( "A2" ) != -1 )
			item->f2.visible = true;
	}
	switch ( fstr[0].unicode() )
	{
	  case 'x':
	  case 'y':
	  case 'r':
	    item->f1.visible = item->f2.visible = false;
	}

	p1 = fstr.indexOf( "D[" );
	if ( p1 != -1 )
	{
		p1 += 2;
		const QString str = fstr.mid( p1, pe - p1);
		p2 = str.indexOf(',');
		p3 = str.indexOf(']');
		if ( p2 > 0 && p2 < p3 )
		{
			tstr = str.left( p2 );
			item->dmin = eval( tstr );
			if ( parserError(false) )
				errflg = true;
			tstr = str.mid( p2 + 1, p3 - p2 - 1 );
			item->dmax = eval( tstr );
			if ( parserError(false) )
				errflg = true;
			if ( item->dmin > item->dmax )
				errflg = true;
		}
		else
			errflg = true;
	}
	p1 = fstr.indexOf( "P[" );
	if ( p1 != -1 )
	{
		int i = 0;
		p1 += 2;
		QString str = fstr.mid( p1, 1000);
		p3 = str.indexOf( ']' );
		do
		{
			p2 = str.indexOf( ',' );
			if ( p2 == -1 || p2 > p3 )
				p2 = p3;
			tstr = str.left( p2++ );
			str = str.mid( p2, 1000 );
			item->parameters.append( ParameterValueItem(tstr, eval( tstr )) );
			if ( parserError(false) )
			{
				errflg = true;
				break;
			}
			p3 -= p2;
		}
		while ( p3 > 0 && i < 10 );
	}

	if ( errflg )
	{
		KMessageBox::error( 0, i18n( "Error in extension." ) );
		return false;
	}
	else
		return true;
}

double XParser::a1fkt( Ufkt *u_item, double x, double h )
{
	return ( fkt(u_item, x + h ) - fkt( u_item, x ) ) / h;
}

double XParser::a2fkt( Ufkt *u_item, double x, double h )
{
	return ( fkt( u_item, x + h + h ) - 2 * fkt( u_item, x + h ) + fkt( u_item, x ) ) / h / h;
}

void XParser::findFunctionName(QString &function_name, int const id, int const type)
{
  char last_character;
  int pos;
  if ( function_name.length()==2/*type == XParser::Polar*/ || type == XParser::ParametricX || type == XParser::ParametricY)
    pos=1;
  else
    pos=0;
  for ( ; ; ++pos)
  {
    last_character = 'f';
    for (bool ok=true; last_character<'x'; ++last_character)
    {
      if ( pos==0 && last_character == 'r') continue;
      function_name[pos]=last_character;
// 	  for( QVector<Ufkt>::iterator it = m_ufkt.begin(); it != m_ufkt.end(); ++it)
	  foreach ( Ufkt * it, m_ufkt )
      {
		  if ( it->fstr().startsWith(function_name+'(') && (int)it->id!=id) //check if the name is free
          ok = false;
      }
      if ( ok) //a free name was found
      {
        //kDebug() << "function_name:" << function_name << endl;
        return;
      }
      ok = true;
    }
    function_name[pos]='f';
    function_name.append('f');
  }
  function_name = "e"; //this should never happen
}

void XParser::fixFunctionName( QString &str, int const type, int const id)
{
  int p1=str.indexOf('(');
  int p2=str.indexOf(')');
  if( p1>=0 && str.at(p2+1)=='=')
  {
    if ( type == XParser::Polar && str.at(0)!='r' )
    {
      if (str.at(0)=='(')
      {
        str.prepend('f');
        p1++;
        p2++;
      }
      str.prepend('r');
      p1++;
      p2++;
    }
    QString const fname = str.left(p1);
//     for ( QVector<Ufkt>::iterator it = ufkt.begin(); it!=ufkt.end(); ++it )
	foreach ( Ufkt * it, m_ufkt )
    {
		if (it->fname() == fname)
      {
        str = str.mid(p1,str.length()-1);
        QString function_name;
        if ( type == XParser::Polar )
          function_name = "rf";
        else if ( type == XParser::ParametricX )
          function_name = "x";
        else if ( type == XParser::ParametricY )
          function_name = "y";
        else
          function_name = "f";
        findFunctionName(function_name, id, type);
        str.prepend( function_name );
        return;
      }
    }
  }
  else if ( p1==-1 || !str.at(p1+1).isLetter() ||  p2==-1 || str.at(p2+1 )!= '=')
  {
    QString function_name;
    if ( type == XParser::Polar )
      function_name = "rf";
    else if ( type == XParser::ParametricX )
      function_name = "xf";
    else if ( type == XParser::ParametricY )
      function_name = "yf";
    else
      function_name = "f";
    str.prepend("(x)=");
    findFunctionName(function_name, id, type);
    str.prepend( function_name );
  }
}

double XParser::euler_method(const double x, const QVector<Ufkt>::iterator it)
{
	double const y = it->oldy + ((x-it->oldx) * it->oldyprim);
	it->oldy = y;
	it->oldx = x;
	it->oldyprim = fkt( it, x ); //yprim;
	return y;
}


int XParser::addfkt( QString fn, Ufkt::Type type )
{
	int id = Parser::addfkt( fn, type );
	Ufkt * ufkt = functionWithID( id );
	if ( ufkt )
		ufkt->f0.color = ufkt->f1.color = ufkt->f2.color = ufkt->integral.color = defaultColor(id);
	
	return id;
}


QColor XParser::defaultColor(int function)
{
	switch ( function % 10 )
	{
		case 0:
			return Settings::color0();
		case 1:
			return Settings::color1();
		case 2:
			return Settings::color2();
		case 3:
			return Settings::color3();
		case 4:
			return Settings::color4();
		case 5:
			return Settings::color5();
		case 6:
			return Settings::color6();
		case 7:
			return Settings::color7();
		case 8:
			return Settings::color8();
		case 9:
			return Settings::color9();
	}
	
	assert( !"Shouldn't happen - XParser::defaultColor" );
}

QStringList XParser::listFunctionNames()
{
	QStringList list;
// 	for( QVector<Ufkt>::iterator it = ufkt.begin(); it != ufkt.end(); ++it)
	foreach ( Ufkt * it, m_ufkt )
	{
		list.append(it->fname());
	}
	return list;	
}

bool XParser::functionFVisible(uint id)
{
	return m_ufkt.contains(id) ? m_ufkt[id]->f0.visible : false;
}
bool XParser::functionF1Visible(uint id)
{
	return m_ufkt.contains(id) ? m_ufkt[id]->f1.visible : false;
}
bool XParser::functionF2Visible(uint id)
{
	return m_ufkt.contains(id) ? m_ufkt[id]->f2.visible : false;
}
bool XParser::functionIntVisible(uint id)
{
	return m_ufkt.contains(id) ? m_ufkt[id]->integral.visible : false;
}

bool XParser::setFunctionFVisible(bool visible, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f0.visible = visible;
	m_modified = true;
	return true;
}
bool XParser::setFunctionF1Visible(bool visible, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f1.visible = visible;
	m_modified = true;
	return true;
}
bool XParser::setFunctionF2Visible(bool visible, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f2.visible = visible;
	m_modified = true;
	return true;
}
bool XParser::setFunctionIntVisible(bool visible, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->integral.visible = visible;
	m_modified = true;
	return true;
}

QString XParser::functionStr(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return "";
	return m_ufkt[id]->fstr();
}

QColor XParser::functionFColor(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return QColor();
	return QColor(m_ufkt[id]->f0.color);
}
QColor XParser::functionF1Color(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return QColor();
	return QColor(m_ufkt[id]->f1.color);
}
QColor XParser::functionF2Color(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return QColor();
	return QColor(m_ufkt[id]->f2.color);
}
QColor XParser::functionIntColor(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return QColor();
	return QColor(m_ufkt[id]->integral.color);
}
bool XParser::setFunctionFColor(const QColor &color, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f0.color = color;
	m_modified = true;
	return true;
}
bool XParser::setFunctionF1Color(const QColor &color, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f0.color = color;
	m_modified = true;
	return true;
}		
bool XParser::setFunctionF2Color(const QColor &color, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f0.color = color;
	m_modified = true;
	return true;
}
bool XParser::setFunctionIntColor(const QColor &color, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f0.color = color;
	m_modified = true;
	return true;
}

double XParser::functionFLineWidth(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
	return m_ufkt[id]->f0.lineWidth;
}
double XParser::functionF1LineWidth(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
	return m_ufkt[id]->f1.lineWidth;
}
double XParser::functionF2LineWidth(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
	return m_ufkt[id]->f2.lineWidth;
}
double XParser::functionIntLineWidth(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
	return m_ufkt[id]->integral.lineWidth;
}
bool XParser::setFunctionFLineWidth(double linewidth, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f0.lineWidth = linewidth;
	m_modified = true;
	return true;
}
bool XParser::setFunctionF1LineWidth(double linewidth, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f1.lineWidth = linewidth;
	m_modified = true;
	return true;
}		
bool XParser::setFunctionF2LineWidth(double linewidth, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->f2.lineWidth = linewidth;
	m_modified = true;
	return true;
}
bool XParser::setFunctionIntLineWidth(double linewidth, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	m_ufkt[id]->integral.lineWidth = linewidth;
	m_modified = true;
	return true;
}

QString XParser::functionMinValue(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
  return m_ufkt[id]->str_dmin;
}

bool XParser::setFunctionMinValue(const QString &min, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
  m_ufkt[id]->str_dmin = min;
  m_modified = true;
  return true;
}

QString XParser::functionMaxValue(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
  return m_ufkt[id]->str_dmax;
}

bool XParser::setFunctionMaxValue(const QString &max, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
  m_ufkt[id]->str_dmax = max;
  m_modified = true;
  return true;
}

QString XParser::functionStartXValue(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
  return m_ufkt[id]->str_startx;
}

bool XParser::setFunctionStartXValue(const QString &x, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
  m_ufkt[id]->str_startx = x;
  m_modified = true;
  return true;
}

QString XParser::functionStartYValue(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return 0;
  return m_ufkt[id]->str_starty;
}

bool XParser::setFunctionStartYValue(const QString &y, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
  m_ufkt[id]->str_starty = y;
  m_modified = true;
  return true;
}

QStringList XParser::functionParameterList(uint id)
{
	if ( !m_ufkt.contains( id ) )
		return QStringList();
	Ufkt *item = m_ufkt[id];
	QStringList str_parameter;
	for ( QList<ParameterValueItem>::iterator it = item->parameters.begin(); it != item->parameters.end(); ++it)
		str_parameter.append( (*it).expression);
	return str_parameter;
}
bool XParser::functionAddParameter(const QString &new_parameter, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	Ufkt *tmp_ufkt = m_ufkt[id];
	for ( QList<ParameterValueItem>::iterator it = tmp_ufkt->parameters.begin(); it != tmp_ufkt->parameters.end(); ++it)
		if ( (*it).expression == new_parameter) //check if the parameter already exists
			return false;

	double const result = eval(new_parameter);
	if ( parserError(false) != 0)
		return false;
	tmp_ufkt->parameters.append( ParameterValueItem(new_parameter,result) );
	m_modified = true;
	return true;
}
bool XParser::functionRemoveParameter(const QString &remove_parameter, uint id)
{
	if ( !m_ufkt.contains( id ) )
		return false;
	Ufkt *tmp_ufkt = m_ufkt[id];
	
	bool found = false;
	QList<ParameterValueItem>::iterator it;
	for ( it = tmp_ufkt->parameters.begin(); it != tmp_ufkt->parameters.end(); ++it)
		if ( (*it).expression == remove_parameter) //check if the parameter already exists
		{
			found = true;
			break;
		}
	if (!found)
		return false;
	tmp_ufkt->parameters.erase(it);
	m_modified = true;
	return true;
}
int XParser::addFunction(const QString &f_str)
{
	QString added_function(f_str);
	int const pos = added_function.indexOf(';');
	if (pos!=-1)
	  added_function = added_function.left(pos);
	
	fixFunctionName(added_function);
	if ( added_function.at(0)== 'x' || added_function.at(0)== 'y') //TODO: Make it possible to define parametric functions
		return -1;
	if  ( added_function.contains('y') != 0)
		return -1;
	Ufkt::Type type = (added_function[0] == 'r') ? Ufkt::Polar : Ufkt::Cartesian;
	
	int const id = addfkt( added_function, type );
	if (id==-1)
		return -1;
	Ufkt *tmp_ufkt = m_ufkt[id];
	if ( pos!=-1 && !getext( tmp_ufkt, f_str ) )
	{
		Parser::delfkt( tmp_ufkt );
		return -1;
	}
	m_modified = true;
	return id;
}

bool XParser::addFunction(const QString &fstr_const, bool f_mode, bool f1_mode, bool f2_mode, bool integral_mode, bool integral_use_precision, double linewidth, double f1_linewidth, double f2_linewidth, double integral_linewidth, const QString &str_dmin, const QString &str_dmax, const QString &str_startx, const QString &str_starty, double integral_precision, QColor color, QColor f1_color, QColor f2_color, QColor integral_color, QStringList str_parameter, int use_slider)
{
	QString fstr(fstr_const);
	Ufkt::Type type;
	switch ( fstr[0].unicode() )
	{
	  case 'r':
	  {
	    fixFunctionName(fstr, XParser::Polar);
		type = Ufkt::Polar;
	    break;
	  }
	  case 'x':
	    fixFunctionName(fstr, XParser::ParametricX);
		type = Ufkt::ParametricX;
	    break;
	  case 'y':
	    fixFunctionName(fstr, XParser::ParametricY);
		type = Ufkt::ParametricY;
	    break;
	  default:
	    fixFunctionName(fstr, XParser::Function);
		type = Ufkt::Cartesian;
	    break;
	}
	int const id = addfkt( fstr, type );
	if ( id==-1 )
		return false;
	Ufkt *added_function = m_ufkt[id];
	added_function->f0.visible = f_mode;
	added_function->f1.visible = f1_mode;
	added_function->f2.visible = f2_mode;
	added_function->integral.visible = integral_mode;
	added_function->integral_use_precision = integral_use_precision;
	added_function->f0.lineWidth = linewidth;
	added_function->f1.lineWidth = f1_linewidth;
	added_function->f2.lineWidth = f2_linewidth;
	added_function->integral.lineWidth = integral_linewidth;
	
  if ( str_dmin.isEmpty() )
    added_function->usecustomxmin = false;
  else //custom minimum range
  {
    added_function->usecustomxmin = true;
    added_function->str_dmin = str_dmin;
    added_function->dmin = eval(str_dmin);
  }
  if ( str_dmax.isEmpty() )
    added_function->usecustomxmax = false;
  else //custom maximum range
  {
    added_function->usecustomxmax = true;
    added_function->str_dmax = str_dmax;
    added_function->dmax = eval(str_dmax);
  }
	added_function->str_startx = str_startx;
	added_function->str_starty = str_starty;
	if ( !str_starty.isEmpty() )
		added_function->starty = eval(str_starty);
	if ( !str_startx.isEmpty() )
		added_function->startx = eval(str_startx);
	added_function->oldx = 0;
	added_function->integral_precision = integral_precision;
	added_function->f0.color = color;
	added_function->f1.color = f1_color;
	added_function->f2.color = f2_color;
	added_function->integral.color = integral_color;
	added_function->use_slider = use_slider;
	for( QStringList::Iterator it = str_parameter.begin(); it != str_parameter.end(); ++it )
	{
		double result = eval(*it);
		if ( parserError(false) != 0)
			continue;
		added_function->parameters.append( ParameterValueItem(*it, result ) );
	}
	m_modified = true;
	return true;
}

bool XParser::setFunctionExpression(const QString &f_str, uint id)
{
	Ufkt * tmp_ufkt = functionWithID( id );
	if ( !tmp_ufkt )
		return false;
	QString const old_fstr = tmp_ufkt->fstr();
	QString const fstr_begin = tmp_ufkt->fstr().left(tmp_ufkt->fstr().indexOf('=')+1);
	
	return tmp_ufkt->setFstr( fstr_begin+f_str );
}

