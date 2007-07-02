/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999, 2000, 2002  Klaus-Dieter Möller <kd.moeller@t-online.de>
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

/**
 * Version string of KmPlot.
 */
#define KP_VERSION "1.2.0"

// local includes
#include "kmplot.h"


// KDE includes
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kapplication.h>


static const char description[] =
    I18N_NOOP( "Mathematical function plotter for KDE" );


int main( int argc, char **argv )
{
	KAboutData aboutData(
	    "kmplot", 0,
	    ki18n( "KmPlot" ),
	    KP_VERSION, ki18n(description), KAboutData::License_GPL,
	    ki18n("(c) 2000-2002, Klaus-Dieter Möller"),
	    KLocalizedString(),
	    "http://edu.kde.org/kmplot/" );
	
	aboutData.addAuthor(
	    ki18n("Klaus-Dieter Möller"), ki18n( "Original Author" ) ,
	    "kdmoeller@foni.net" );
	aboutData.addAuthor(
	    ki18n("Matthias Meßmer"), ki18n( "GUI" ) ,
	    "bmlmessmer@web.de" );
	aboutData.addAuthor( ki18n("Fredrik Edemar"), ki18n( "Various improvements" ), "f_edemar@linux.se" );
	aboutData.addAuthor( ki18n("David Saxton"), ki18n( "Porting to Qt 4, UI improvements, features" ), "david@bluehaze.org" );
	
	aboutData.addCredit( ki18n("David Vignoni"), ki18n( "svg icon" ), "david80v@tin.it" );
	aboutData.addCredit( ki18n("Albert Astals Cid"), ki18n( "command line options, MIME type" ), "tsdgeos@terra.es" );

	KCmdLineArgs::init( argc, argv, &aboutData );

	KCmdLineOptions options;
	options.add("f");
	options.add("function <argument>", ki18n( "Initial functions to plot" ));
	options.add("+[URL]", ki18n( "File to open" ));
	KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

	KApplication ka;
	KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
	new KmPlot( args );
	QObject::connect(kapp, SIGNAL(lastWindowClosed()), kapp, SLOT(quit()));
	return ka.exec();
}

