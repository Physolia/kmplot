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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
*/

#ifndef KPARAMETEREDITOR_H
#define KPARAMETEREDITOR_H

#include "qparametereditor.h"
//Added by qt3to4:
#include <Q3ValueList>
#include "xparser.h"

class ParameterValueList;

/**
@author Fredrik Edemar
*/
/// This class handles the parameter values: it can create, remove, edit and import values.
class KParameterEditor : public QParameterEditor
{
Q_OBJECT
public:
    KParameterEditor(XParser *, Q3ValueList<ParameterValueItem> *, QWidget *parent = 0, const char *name = 0);
    ~KParameterEditor();
    
public slots:
    void cmdNew_clicked();
    void cmdEdit_clicked();
    void cmdDelete_clicked();
    void cmdImport_clicked();
    void cmdExport_clicked();
    void varlist_clicked( Q3ListBoxItem *  );
    void varlist_doubleClicked( Q3ListBoxItem * );
  
    
private:
    /// Check so that it doesn't exist two equal values
    bool checkTwoOfIt( const QString & text);
    Q3ValueList<ParameterValueItem> *m_parameter;
    XParser *m_parser;
};

#endif
