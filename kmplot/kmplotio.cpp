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
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

// Qt includes
#include <qdom.h>
#include <qfile.h>

// KDE includes
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>

// ANSI-C includes
#include <stdlib.h>

// local includes
#include "kmplotio.h"
#include "MainDlg.h"
#include "settings.h"

class XParser;

KmPlotIO::KmPlotIO()
{
}


KmPlotIO::~KmPlotIO()
{
}

bool KmPlotIO::save(  XParser *parser, const KURL &url )
{
	// saving as xml by a QDomDocument
	QDomDocument doc( "kmpdoc" );
	// the root tag
	QDomElement root = doc.createElement( "kmpdoc" );
	root.setAttribute( "version", "1" );
	doc.appendChild( root );

	// the axes tag
	QDomElement tag = doc.createElement( "axes" );

	tag.setAttribute( "color", Settings::axesColor().name() );
	tag.setAttribute( "width", Settings::axesLineWidth() );
	tag.setAttribute( "tic-width", Settings::ticWidth() );
	tag.setAttribute( "tic-legth", Settings::ticLength() );

	addTag( doc, tag, "show-axes", Settings::showAxes() ? "1" : "-1" );
	addTag( doc, tag, "show-arrows", Settings::showArrows() ? "1" : "-1" );
	addTag( doc, tag, "show-label", Settings::showLabel() ? "1" : "-1" );
	addTag( doc, tag, "show-frame", Settings::showExtraFrame() ? "1" : "-1" );
	addTag( doc, tag, "show-extra-frame", Settings::showExtraFrame() ? "1" : "-1" );
	
	addTag( doc, tag, "xcoord", QString::number( Settings::xRange() ) );
	if( Settings::xRange() == 4 ) // custom plot range
	{
		addTag( doc, tag, "xmin", Settings::xMin() );
		addTag( doc, tag, "xmax", Settings::xMax() );
	}
	
	addTag( doc, tag, "ycoord", QString::number( Settings::yRange() ) );
	if( Settings::yRange() == 4 ) // custom plot range
	{
		addTag( doc, tag, "ymin", Settings::yMin() );
		addTag( doc, tag, "ymax", Settings::yMax() );
	}

	root.appendChild( tag );

	tag = doc.createElement( "grid" );

	tag.setAttribute( "color", Settings::gridColor().name() );
	tag.setAttribute( "width", Settings::gridLineWidth() );

	addTag( doc, tag, "mode", QString::number( Settings::gridStyle() ) );

	root.appendChild( tag );

	tag = doc.createElement( "scale" );

	QString temp;
	temp.setNum(Settings::xScaling());
	addTag( doc, tag, "tic-x", temp );
	temp.setNum(Settings::yScaling());
	addTag( doc, tag, "tic-y", temp );
	temp.setNum(Settings::xPrinting());
	addTag( doc, tag, "print-tic-x", temp );
	temp.setNum(Settings::yPrinting());
	addTag( doc, tag, "print-tic-y", temp);
	
	root.appendChild( tag );

	for ( int ix = 0; ix < parser->ufanz; ix++ )
	{
		if ( !parser->fktext[ ix ].extstr.isEmpty() )
		{
			tag = doc.createElement( "function" );

			tag.setAttribute( "number", ix );
			tag.setAttribute( "visible", parser->fktext[ ix ].f_mode );
			tag.setAttribute( "color", QColor( parser->fktext[ ix ].color ).name() );
			tag.setAttribute( "width", parser->fktext[ ix ].linewidth );
			tag.setAttribute( "use-slider", parser->fktext[ ix ].use_slider );
			
			if ( parser->fktext[ ix ].f1_mode)
			{
				tag.setAttribute( "visible-deriv", parser->fktext[ ix ].f1_mode );
				tag.setAttribute( "deriv-color", QColor( parser->fktext[ ix ].f1_color ).name() );
				tag.setAttribute( "deriv-width", parser->fktext[ ix ].f1_linewidth );	
			}
			
			if ( parser->fktext[ ix ].f2_mode)
			{
				tag.setAttribute( "visible-2nd-deriv", parser->fktext[ ix ].f2_mode );
				tag.setAttribute( "deriv2nd-color", QColor( parser->fktext[ ix ].f2_color ).name() );
				tag.setAttribute( "deriv2nd-width", parser->fktext[ ix ].f2_linewidth );
			}
			
			if ( parser->fktext[ ix ].integral_mode)
			{
				tag.setAttribute( "visible-integral", "1" );
				tag.setAttribute( "integral-color", QColor( parser->fktext[ ix ].integral_color ).name() );
				tag.setAttribute( "integral-width", parser->fktext[ ix ].integral_linewidth );
				tag.setAttribute( "integral-use-precision", parser->fktext[ ix ].integral_use_precision );
				tag.setAttribute( "integral-precision", parser->fktext[ ix ].integral_precision );
				tag.setAttribute( "integral-startx", parser->fktext[ ix ].str_startx );
				tag.setAttribute( "integral-starty", parser->fktext[ ix ].str_starty );
			}
			
			addTag( doc, tag, "equation", parser->fktext[ ix ].extstr );
			
			if( parser->fktext[ ix ].k_anz > 0 )
				addTag( doc, tag, "parameterlist", parser->fktext[ ix ].str_parameter.join( "," ) );
			
			addTag( doc, tag, "arg-min", parser->fktext[ ix ].str_dmin );
			addTag( doc, tag, "arg-max", parser->fktext[ ix ].str_dmax );
						
			root.appendChild( tag );
			
		}
	}
	
	tag = doc.createElement( "fonts" );
	addTag( doc, tag, "axes-font", Settings::axesFont() );
	addTag( doc, tag, "header-table-font", Settings::headerTableFont() );
	root.appendChild( tag );

        QFile xmlfile;
        if (!url.isLocalFile() )
        {
                 KTempFile tmpfile;
                 xmlfile.setName(tmpfile.name() );
                 if (!xmlfile.open( IO_WriteOnly ) )
                 {
                        tmpfile.unlink();
                        return false;
                 }
                 QTextStream ts( &xmlfile );
                 doc.save( ts, 4 );
                 xmlfile.close();
                 
                 if ( !KIO::NetAccess::upload(tmpfile.name(), url,0))
                 {
                        tmpfile.unlink();
                        return false; 
                 }
                 tmpfile.unlink();
        }
        else
        {
                xmlfile.setName(url.prettyURL(0,KURL::StripFileProtocol)  );
                if (!xmlfile.open( IO_WriteOnly ) )
                        return false;
                QTextStream ts( &xmlfile );
                doc.save( ts, 4 );
                xmlfile.close();
                return true;       
        }
        return true;
 
}

void KmPlotIO::addTag( QDomDocument &doc, QDomElement &parentTag, const QString tagName, const QString tagValue )
{
	QDomElement tag = doc.createElement( tagName );
	QDomText value = doc.createTextNode( tagValue );
	tag.appendChild( value );
	parentTag.appendChild( tag );
}

bool KmPlotIO::load( XParser *parser, const KURL &url )
{
	QDomDocument doc( "kmpdoc" );
        QFile f;
        if ( !url.isLocalFile() )
        {
                if( !KIO::NetAccess::exists( url, true, 0 ) )
                {
                        KMessageBox::error(0,i18n("The file doesn't exist."));
                        return false;
                }
                QString tmpfile;
                if( !KIO::NetAccess::download( url, tmpfile, 0 ) )
                {
                        KMessageBox::error(0,i18n("An error appeared when opening this file"));
                        return false;
                }
                f.setName(tmpfile);
        }
        else
	       f.setName( url.prettyURL(0,KURL::StripFileProtocol) );
        
	if ( !f.open( IO_ReadOnly ) )
	{
		KMessageBox::error(0,i18n("An error appeared when opening this file"));
		return false;
	}
	if ( !doc.setContent( &f ) )
	{
		KMessageBox::error(0,i18n("The file could not be loaded"));
		f.close();
		return false;
	}
	f.close();

	QDomElement element = doc.documentElement();
	QString version = element.attribute( "version" );
	if ( version == QString::null) //an old kmplot-file
	{
		MainDlg::oldfileversion = true;
		for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() )
		{
			if ( n.nodeName() == "axes" )
				oldParseAxes( n.toElement() );
			if ( n.nodeName() == "grid" )
				parseGrid( n.toElement() );
			if ( n.nodeName() == "scale" )
				oldParseScale( n.toElement() );
			if ( n.nodeName() == "function" )
				oldParseFunction( parser, n.toElement() );
		}
	}
	else if (version == "1")
	{
		for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() )
		{
			if ( n.nodeName() == "axes" )
				parseAxes( n.toElement() );
			if ( n.nodeName() == "grid" )
				parseGrid( n.toElement() );
			if ( n.nodeName() == "scale" )
				parseScale( n.toElement() );
			if ( n.nodeName() == "function" )
				parseFunction( parser, n.toElement() );
		}
	}
	else
		KMessageBox::error(0,i18n("The file had an unknown version number"));
                
        if ( !url.isLocalFile() )
                KIO::NetAccess::removeTempFile( f.name() );
	return true;
}

void KmPlotIO::parseAxes( const QDomElement &n )
{
	Settings::setAxesLineWidth( n.attribute( "width", "1" ).toInt() );
	Settings::setAxesColor( QColor( n.attribute( "color", "#000000" ) ) );
	Settings::setTicWidth( n.attribute( "tic-width", "3" ).toInt() );
	Settings::setTicLength( n.attribute( "tic-length", "10" ).toInt() );

	Settings::setShowAxes( n.namedItem( "show-axes" ).toElement().text().toInt() == 1 );
	Settings::setShowArrows( n.namedItem( "show-arrows" ).toElement().text().toInt() == 1 );
	Settings::setShowLabel( n.namedItem( "show-label" ).toElement().text().toInt() == 1 );
	Settings::setShowFrame( n.namedItem( "show-frame" ).toElement().text().toInt() == 1 );
	Settings::setShowExtraFrame( n.namedItem( "show-extra-frame" ).toElement().text().toInt() == 1 );
	Settings::setXRange( n.namedItem( "xcoord" ).toElement().text().toInt() );
	Settings::setXMin( n.namedItem( "xmin" ).toElement().text() );
	Settings::setXMax( n.namedItem( "xmax" ).toElement().text() );
	Settings::setYRange( n.namedItem( "ycoord" ).toElement().text().toInt() );
	Settings::setYMin( n.namedItem( "ymin" ).toElement().text() );
	Settings::setYMax( n.namedItem( "ymax" ).toElement().text() );
}

void KmPlotIO::parseGrid( const QDomElement & n )
{
	Settings::setGridColor( QColor( n.attribute( "color", "#c0c0c0" ) ) );
	Settings::setGridLineWidth( n.attribute( "width", "1" ).toInt() );

	Settings::setGridStyle( n.namedItem( "mode" ).toElement().text().toInt() );
}

int unit2index( const QString unit )
{
	QString units[ 9 ] = { "10", "5", "2", "1", "0.5", "pi/2", "pi/3", "pi/4",i18n("automatic") };
	int index = 0;
	while( ( index < 9 ) && ( unit!= units[ index ] ) ) index ++;
	if( index == 9 ) index = -1;
	return index;
}


void KmPlotIO::parseScale( const QDomElement & n )
{
	Settings::setXScaling( atoi( n.namedItem( "tic-x" ).toElement().text().latin1() ) );
	Settings::setYScaling( atoi( n.namedItem( "tic-y" ).toElement().text().latin1() ) );
	Settings::setXPrinting( atoi( n.namedItem( "print-tic-x" ).toElement().text().latin1() ) );
	Settings::setYPrinting( atoi( n.namedItem( "print-tic-y" ).toElement().text().latin1() ) );
}


void KmPlotIO::parseFunction(  XParser *parser, const QDomElement & n )
{
	int ix = n.attribute( "number" ).toInt();
	QString temp;
	parser->fktext[ ix ].f_mode = n.attribute( "visible" ).toInt();
	parser->fktext[ ix ].color = QColor( n.attribute( "color" ) ).rgb();
	parser->fktext[ ix ].linewidth = n.attribute( "width" ).toInt();
	parser->fktext[ ix ].use_slider = n.attribute( "use-slider" ).toInt();
	
	temp = n.attribute( "visible-deriv" );
	if (temp != QString::null)
	{
		parser->fktext[ ix ].f1_mode = temp.toInt();
		parser->fktext[ ix ].f1_color = QColor(n.attribute( "deriv-color" )).rgb();
		parser->fktext[ ix ].f1_linewidth = n.attribute( "deriv-width" ).toInt();
	}
	else
	{
		parser->fktext[ ix ].f1_mode = 0;
		parser->fktext[ ix ].f1_color = parser->fktext[ ix ].color0;
		parser->fktext[ ix ].f1_linewidth = parser->linewidth0;
	}
		
	temp = n.attribute( "visible-2nd-deriv" );
	if (temp != QString::null)
	{
		parser->fktext[ ix ].f2_mode = temp.toInt();
		parser->fktext[ ix ].f2_color = QColor(n.attribute( "deriv2nd-color" )).rgb();
		parser->fktext[ ix ].f2_linewidth = n.attribute( "deriv2nd-width" ).toInt();
	}
	else
	{
		parser->fktext[ ix ].f2_mode = 0;
		parser->fktext[ ix ].f2_color = parser->fktext[ ix ].color0;
		parser->fktext[ ix ].f2_linewidth = parser->linewidth0;
	}
	
	temp = n.attribute( "visible-integral" );
	if (temp != QString::null)
	{
		parser->fktext[ ix ].integral_mode = temp.toInt();
		parser->fktext[ ix ].integral_color = QColor(n.attribute( "integral-color" )).rgb();
		parser->fktext[ ix ].integral_linewidth = n.attribute( "integral-width" ).toInt();
		parser->fktext[ ix ].integral_use_precision = n.attribute( "integral-use-precision" ).toInt();
		parser->fktext[ ix ].integral_precision = n.attribute( "integral-precision" ).toInt();
		parser->fktext[ ix ].str_startx = n.attribute( "integral-startx" );
		parser->fktext[ ix ].startx = parser->eval( parser->fktext[ ix ].str_startx );
		parser->fktext[ ix ].str_starty = n.attribute( "integral-starty" );
		parser->fktext[ ix ].starty = parser->eval( parser->fktext[ ix ].str_starty );
		
	}
	else
	{
		parser->fktext[ ix ].integral_mode = 0;
		parser->fktext[ ix ].integral_color = parser->fktext[ ix ].color0;
		parser->fktext[ ix ].integral_linewidth = parser->linewidth0;
		parser->fktext[ ix ].integral_use_precision = 0;
		parser->fktext[ ix ].integral_precision = parser->fktext[ ix ].linewidth;
	}
	
	parser->fktext[ ix ].extstr = n.namedItem( "equation" ).toElement().text();
	QCString fstr = parser->fktext[ ix ].extstr.utf8();
	if ( !fstr.isEmpty() )
	{
		int i = fstr.find( ';' );
		QCString str;
		if ( i == -1 )
			str = fstr;
		else
			str = fstr.left( i );
		QString f_str (str);
		ix = parser->addfkt( str );
		parseParameters( parser, n, ix );
		parser->getext( ix );
	}
	parser->fktext[ ix ].str_dmin = n.namedItem( "arg-min" ).toElement().text();
	if( parser->fktext[ ix ].str_dmin.isEmpty() ) parser->fktext[ ix ].str_dmin = "0.0";
	else parser->fktext[ ix ].dmin = parser->eval( parser->fktext[ ix ].str_dmin );
	parser->fktext[ ix ].str_dmax = n.namedItem( "arg-max" ).toElement().text();
	if( parser->fktext[ ix ].str_dmax.isEmpty() ) parser->fktext[ ix ].str_dmax = "0.0";
	else parser->fktext[ ix ].dmax = parser->eval( parser->fktext[ ix ].str_dmax );
}

void KmPlotIO::parseParameters( XParser *parser, const QDomElement &n, int ix )
{
	parser->fktext[ ix ].str_parameter = QStringList::split( ",", n.namedItem( "parameterlist" ).toElement().text() );
	
	parser->fktext[ ix ].k_anz = 0;
	for( QStringList::Iterator it = parser->fktext[ ix ].str_parameter.begin(); it != parser->fktext[ ix ].str_parameter.end(); ++it )
	{
		parser->fktext[ ix ].k_liste[ parser->fktext[ ix ].k_anz ] = parser->eval( *it );
		parser->fktext[ ix ].k_anz++;
	}
	
}
void KmPlotIO::oldParseFunction(  XParser *parser, const QDomElement & n )
{
	kdDebug() << "parsing old function" << endl;

	int ix = n.attribute( "number" ).toInt();
	parser->fktext[ ix ].f_mode = n.attribute( "visible" ).toInt();
	parser->fktext[ ix ].f1_mode = n.attribute( "visible-deriv" ).toInt();
	parser->fktext[ ix ].f2_mode = n.attribute( "visible-2nd-deriv" ).toInt();
	parser->fktext[ ix ].linewidth = n.attribute( "width" ).toInt();
	parser->fktext[ ix ].color = QColor( n.attribute( "color" ) ).rgb();
	parser->fktext[ ix ].f1_color = parser->fktext[ ix ].color;
	parser->fktext[ ix ].f2_color = parser->fktext[ ix ].color;
	parser->fktext[ ix ].integral_color = parser->fktext[ ix ].color;

	parser->fktext[ ix ].extstr = n.namedItem( "equation" ).toElement().text();
	QCString fstr = parser->fktext[ ix ].extstr.utf8();
	if ( !fstr.isEmpty() )
	{
		int i = fstr.find( ';' );
		QCString str;
		if ( i == -1 )
			str = fstr;
		else
			str = fstr.left( i );
		ix = parser->addfkt( str );
		parser->getext( ix );
	}
}

void KmPlotIO::oldParseAxes( const QDomElement &n )
{
	Settings::setAxesLineWidth( n.attribute( "width", "1" ).toInt() );
	Settings::setAxesColor( QColor( n.attribute( "color", "#000000" ) ) );
	Settings::setTicWidth( n.attribute( "tic-width", "3" ).toInt() );
	Settings::setTicLength( n.attribute( "tic-length", "10" ).toInt() );

	Settings::setShowAxes( true );
	Settings::setShowArrows( true );
	Settings::setShowLabel( true );
	Settings::setShowFrame( true );
	Settings::setShowExtraFrame( true );
	Settings::setXRange( n.namedItem( "xcoord" ).toElement().text().toInt() );
	Settings::setXMin( n.namedItem( "xmin" ).toElement().text() );
	Settings::setXMax( n.namedItem( "xmax" ).toElement().text() );
	Settings::setYRange( n.namedItem( "ycoord" ).toElement().text().toInt() );
	Settings::setYMin( n.namedItem( "ymin" ).toElement().text() );
	Settings::setYMax( n.namedItem( "ymax" ).toElement().text() );
}

void KmPlotIO::oldParseScale( const QDomElement & n )
{
	Settings::setXScaling( unit2index( n.namedItem( "tic-x" ).toElement().text() ) );
	Settings::setYScaling( unit2index( n.namedItem( "tic-y" ).toElement().text() ) );
	Settings::setXPrinting( unit2index( n.namedItem( "print-tic-x" ).toElement().text() ) );
	Settings::setYPrinting( unit2index( n.namedItem( "print-tic-y" ).toElement().text() ) );
}