/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 2004  Fredrik Edemar
*                     f_edemar@linux.se
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

#ifndef KMPLOTIFACE_H
#define KMPLOTIFACE_H


#include <dcopobject.h>
#include <kurl.h>

/**
@author Fredrik Edemar
*/
/// All functions in KmPlotIface are accessible with DCOP. For descriptions about the functions, see KmPlot.
class KmPlotIface : virtual public DCOPObject
{
	K_DCOP
k_dcop:

	virtual void setStatusBarText(const QString &text, int id) = 0;
	virtual void optionsConfigureKeys() = 0;
	virtual void optionsConfigureToolbars() = 0;
	virtual void openFileInNewWindow(const KURL url) = 0;

	virtual bool stopProgressBar() = 0;
	virtual void startProgressBar(int) = 0;
	virtual void increaseProgressBar() = 0;
};


#endif


