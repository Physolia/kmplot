/*
* KmPlot - a math. function plotter for the KDE-Desktop
*
* Copyright (C) 1998, 1999, 2000, 2002  Klaus-Dieter M�ler <kd.moeller@t-online.de>
*                                 2006  David Saxton <david@bluehaze.org>
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

// Qt includes
#include <QMainWindow>
#include <QPixmap>
#include <qslider.h>
#include <QTimer>

// KDE includes
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kio/netaccess.h>
#include <kinstance.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <ktempfile.h>
#include <ktoolbar.h>
#include <ktoolinvocation.h>

// local includes
#include "functioneditor.h"
#include "kprinterdlg.h"
#include "kconstanteditor.h"
#include "MainDlg.h"
#include "MainDlg.moc"
#include "settings.h"
#include "ui_settingspagecolor.h"
#include "ui_settingspagefonts.h"
#include "ksliderwindow.h"

#include <assert.h>

class XParser;
class KmPlotIO;

bool MainDlg::oldfileversion;
MainDlg * MainDlg::m_self = 0;


MainDlg::MainDlg(QWidget *parentWidget, QObject *parent, const QStringList& ) :
		DCOPObject( "MainDlg" ),
		KParts::ReadOnlyPart( parent ),
		m_recentFiles( 0 ),
		m_modified(false),
		m_parent(parentWidget)
{
	assert( !m_self ); // this class should only be constructed once
	m_self = this;
	
	// we need an instance
	setInstance( KmPlotPartFactory::instance() );

	kDebug() << "parentWidget->objectName():" << parentWidget->objectName() << endl;
	if ( QString(parentWidget->objectName()).startsWith("KmPlot") )
	{
		setXMLFile("kmplot_part.rc");
		m_readonly = false;
	}
	else
	{
		setXMLFile("kmplot_part_readonly.rc");
		m_readonly = true;
		new BrowserExtension(this); // better integration with Konqueror
	}
	
	coordsDialog = 0;
	m_popupmenu = new KMenu( parentWidget );
	m_newPlotMenu = new KMenu( parentWidget );
	(void) new View( m_readonly, m_modified, m_popupmenu, parentWidget, actionCollection() );
	connect( View::self(), SIGNAL( setStatusBarText(const QString &)), this, SLOT( setReadOnlyStatusBarText(const QString &) ) );
	
	if ( !m_readonly )
	{
		m_functionEditor = new FunctionEditor( m_newPlotMenu, parentWidget );
		static_cast<QMainWindow*>(parentWidget)->addDockWidget( Qt::LeftDockWidgetArea, m_functionEditor );
	}
	
	setWidget( View::self() );
	View::self()->setFocusPolicy(Qt::ClickFocus);
	minmaxdlg = new KMinMax(m_parent);
	View::self()->setMinMaxDlg(minmaxdlg);
	setupActions();
	XParser::self()->constants()->load();
	kmplotio = new KmPlotIO();
	m_config = KGlobal::config();
	m_recentFiles->loadEntries( m_config );
	
	
	//BEGIN undo/redo stuff
	m_currentState = kmplotio->currentState();
	m_saveCurrentStateTimer = new QTimer( this );
	m_saveCurrentStateTimer->setSingleShot( true );
	connect( m_saveCurrentStateTimer, SIGNAL(timeout()), this, SLOT(saveCurrentState()) );
	//END undo/redo stuff
	

	// Let's create a Configure Diloag
	m_settingsDialog = new KConfigDialog( parentWidget, "settings", Settings::self() );
	m_settingsDialog->setHelp("general-config");

	// create and add the page(s)
	m_generalSettings = new SettingsPageGeneral( View::self() );
	m_colorSettings = new SettingsPageColor( View::self() );
	m_fontsSettings = new SettingsPageFonts( View::self() );
	m_constantsSettings = new KConstantEditor( 0 );
	m_constantsSettings->setObjectName( "constantsSettings" );
	
	m_settingsDialog->addPage( m_generalSettings, i18n("General"), "package_settings", i18n("General Settings") );
	m_settingsDialog->addPage( m_colorSettings, i18n("Colors"), "colorize", i18n("Colors") );
	m_settingsDialog->addPage( m_fontsSettings, i18n("Fonts"), "font", i18n("Fonts") );
	m_settingsDialog->addPage( m_constantsSettings, i18n("Constants"), "editconstants", i18n("Constants") );
	// User edited the configuration - update your local copies of the
	// configuration data
	connect( m_settingsDialog, SIGNAL( settingsChanged( const QString &) ), this, SLOT(updateSettings() ) );
}

MainDlg::~MainDlg()
{
	m_recentFiles->saveEntries( m_config );
	XParser::self()->constants()->save();
	delete kmplotio;
}

void MainDlg::setupActions()
{
	// standard actions
	m_recentFiles = KStdAction::openRecent( this, SLOT( slotOpenRecent( const KUrl& ) ), actionCollection(),"file_openrecent");
	KStdAction::print( this, SLOT( slotPrint() ), actionCollection(),"file_print" );
	KStdAction::save( this, SLOT( slotSave() ), actionCollection() );
	KStdAction::saveAs( this, SLOT( slotSaveas() ), actionCollection() );
	connect( kapp, SIGNAL( lastWindowClosed() ), kapp, SLOT( quit() ) );

	KAction *prefs  = KStdAction::preferences( this, SLOT( slotSettings() ), actionCollection());
	prefs->setText( i18n( "Configure KmPlot..." ) );
	KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
	KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());


	// KmPlot specific actions
	
	//BEGIN file menu
	KAction * exportAction = new KAction( i18n( "E&xport..." ), actionCollection(), "export" );
	connect( exportAction, SIGNAL(triggered(bool)), this, SLOT( slotExport() ) );
	//END file menu

	
	//BEGIN edit menu
	m_undoAction = KStdAction::undo( this, SLOT(undo()), actionCollection() );
	m_undoAction->setEnabled( false );
	
	m_redoAction = KStdAction::redo( this, SLOT(redo()), actionCollection() );
	m_redoAction->setEnabled( false );
	
	KAction * editAxes = new KAction( i18n( "&Coordinate System..." ), actionCollection(), "editaxes" );
	editAxes->setIcon( KIcon("coords.png") );
	connect( editAxes, SIGNAL(triggered(bool)), this, SLOT( editAxes() ) );
	
	KAction * editScaling = new KAction( i18n( "&Scaling..." ), actionCollection(), "editscaling" );
	editScaling->setIcon( KIcon("scaling") );
	connect( editScaling, SIGNAL(triggered(bool)), this, SLOT( editScaling() ) );
	//END edit menu
	
	
	//BEGIN view menu
	/// \todo check that new shortcuts work
	
	KAction * zoomIn = new KAction( i18n("Zoom &In"), actionCollection(), "zoom_in" );
	zoomIn->setShortcut( Qt::ControlModifier | Qt::Key_1 );
	zoomIn->setIcon( KIcon("viewmag+") );
	connect( zoomIn, SIGNAL(triggered(bool)), View::self(), SLOT(mnuZoomIn_clicked()) );
	
	KAction * zoomOut = new KAction( i18n("Zoom &Out"), actionCollection(),"zoom_out" );
	zoomOut->setShortcut( Qt::ControlModifier | Qt::Key_2 );
	zoomOut->setIcon( KIcon("viewmag-") );
	connect( zoomOut, SIGNAL(triggered(bool)), View::self(), SLOT( mnuZoomOut_clicked() ) );
	
	KAction * zoomTrig = new KAction( i18n("&Fit Widget to Trigonometric Functions"), actionCollection(), "zoom_trig" );
	connect( zoomTrig, SIGNAL(triggered(bool)), View::self(), SLOT( mnuTrig_clicked() ) );
	
	KAction * coordI = new KAction( i18n( "Coordinate System I" ), actionCollection(), "coord_i" );
	coordI->setIcon( KIcon("ksys1.png") );
	connect( coordI, SIGNAL(triggered(bool)), this, SLOT( slotCoord1() ) );
	
	KAction * coordII = new KAction( i18n( "Coordinate System II" ), actionCollection(), "coord_ii" );
	coordII->setIcon( KIcon("ksys2.png") );
	connect( coordII, SIGNAL(triggered(bool)), this, SLOT( slotCoord2() ) );
	
	KAction * coordIII = new KAction( i18n( "Coordinate System III" ), actionCollection(), "coord_iii" );
	coordIII->setIcon( KIcon("ksys3.png") );
	connect( coordIII, SIGNAL(triggered(bool)), this, SLOT( slotCoord3() ) );
	//END view menu
	
	
	//BEGIN tools menu
	KAction *mnuYValue =  new KAction( i18n( "&Get y-Value..." ), actionCollection(), "yvalue" );
	connect( mnuYValue, SIGNAL(triggered(bool)), this, SLOT( getYValue() ) );
	
	KAction *mnuMinValue = new KAction( i18n( "&Search for Minimum Value..." ), actionCollection(), "minimumvalue" );
	mnuMinValue->setIcon( KIcon("minimum") );
	connect( mnuMinValue, SIGNAL(triggered(bool)), this, SLOT( findMinimumValue() ) );
	
	KAction *mnuMaxValue = new KAction( i18n( "&Search for Maximum Value..." ), actionCollection(), "maximumvalue" );
	mnuMaxValue->setIcon( KIcon("maximum") );
	connect( mnuMaxValue, SIGNAL(triggered(bool)), this, SLOT( findMaximumValue() ) );
	
	KAction *mnuArea = new KAction( i18n( "&Area Under Graph..." ), actionCollection(), "grapharea" );
	connect( mnuArea, SIGNAL(triggered(bool)),this, SLOT( graphArea() )  );
	//END tools menu

	
	//BEGIN help menu
	KAction * namesAction = new KAction( i18n( "Predefined &Math Functions" ), actionCollection(), "names" );
	namesAction->setIcon( KIcon("functionhelp") );
	connect( namesAction, SIGNAL(triggered(bool)), this, SLOT( slotNames() ) );
	//END help menu
	
	
	//BEGIN new plots menu
	KAction * newFunction = new KAction( i18n( "Cartesian Plot" ), actionCollection(), "newcartesian" );
	newFunction->setIcon( KIcon("newfunction") );
	connect( newFunction, SIGNAL(triggered(bool)), m_functionEditor, SLOT( createCartesian() ) );
	m_newPlotMenu->addAction( newFunction );
        
	KAction * newParametric = new KAction( i18n( "Parametric Plot" ), actionCollection(), "newparametric" );
	newParametric->setIcon( KIcon("newparametric") );
	connect( newParametric, SIGNAL(triggered(bool)), m_functionEditor, SLOT( createParametric() ) );
	m_newPlotMenu->addAction( newParametric );
        
	KAction * newPolar = new KAction( i18n( "Polar Plot" ), actionCollection(), "newpolar" );
	newPolar->setIcon( KIcon("newpolar") );
	connect( newPolar, SIGNAL(triggered(bool)), m_functionEditor, SLOT( createPolar() ) );
	m_newPlotMenu->addAction( newPolar );
	//END new plots menu
	
	

	View::self()->m_menuSliderAction = new KToggleAction( i18n( "Show Sliders" ), actionCollection(), "options_configure_show_sliders" );
	connect( View::self()->m_menuSliderAction, SIGNAL(triggered(bool)), this, SLOT( toggleShowSliders() ) );
	

	// Popup menu
	KAction *mnuHide = new KAction(i18n("&Hide"), actionCollection(),"mnuhide" );
	connect( mnuHide, SIGNAL(triggered(bool)), View::self(), SLOT( mnuHide_clicked() ) );
	m_popupmenu->addAction( mnuHide );
	
	KAction *mnuRemove = new KAction(i18n("&Remove"), actionCollection(),"mnuremove"  );
	mnuRemove->setIcon( KIcon("editdelete") );
	connect( mnuRemove, SIGNAL(triggered(bool)), View::self(), SLOT( mnuRemove_clicked() ) );
	m_popupmenu->addAction( mnuRemove );
	
	KAction *mnuEdit = new KAction(i18n("&Edit"), actionCollection(),"mnuedit"  );
	mnuEdit->setIcon( KIcon("editplots") );
	connect(mnuEdit , SIGNAL(triggered(bool)), View::self(), SLOT( mnuEdit_clicked() ) );
	m_popupmenu->addAction( mnuEdit );
	
	m_popupmenu->addSeparator();
	m_popupmenu->addAction( mnuYValue );
	m_popupmenu->addAction( mnuMinValue );
	m_popupmenu->addAction( mnuMaxValue );
	m_popupmenu->addAction( mnuArea );
}


void MainDlg::undo()
{
	kDebug() << k_funcinfo << endl;
	
	if ( m_undoStack.isEmpty() )
		return;
	
	m_redoStack.push( m_currentState );
	m_currentState = m_undoStack.pop();
	
	kmplotio->restore( m_currentState );
	View::self()->drawPlot();
	
	m_undoAction->setEnabled( !m_undoStack.isEmpty() );
	m_redoAction->setEnabled( true );
}


void MainDlg::redo()
{
	kDebug() << k_funcinfo << endl;
	
	if ( m_redoStack.isEmpty() )
		return;
	
	m_undoStack.push( m_currentState );
	m_currentState = m_redoStack.pop();
	
	kmplotio->restore( m_currentState );
	View::self()->drawPlot();
	
	m_undoAction->setEnabled( true );
	m_redoAction->setEnabled( !m_redoStack.isEmpty() );
}


void MainDlg::requestSaveCurrentState()
{
	m_saveCurrentStateTimer->start( 0 );
}
void MainDlg::saveCurrentState( )
{
	kDebug() << k_funcinfo << endl;
	
	m_redoStack.clear();
	m_undoStack.push( m_currentState );
	m_currentState = kmplotio->currentState();
	
	// limit stack size to 100 items
	while ( m_undoStack.count() > 100 )
		m_undoStack.pop_front();
	
	m_undoAction->setEnabled( true );
	m_redoAction->setEnabled( false );
	
	m_modified = true;
}


bool MainDlg::checkModified()
{
	if( m_modified )
	{
		int saveit = KMessageBox::warningYesNoCancel( m_parent, i18n( "The plot has been modified.\n"
		             "Do you want to save it?" ), QString(), KStdGuiItem::save(), KStdGuiItem::discard() );
		switch( saveit )
		{
			case KMessageBox::Yes:
				slotSave();
				if ( m_modified) // the user didn't saved the file
					return false;
				break;
			case KMessageBox::Cancel:
				return false;
		}
	}
	return true;
}


void MainDlg::slotSave()
{
	if ( !m_modified || m_readonly) //don't save if no changes are made or readonly is enabled
		return;
	if ( m_url.isEmpty() )            // if there is no file name set yet
		slotSaveas();
	else
	{
		if ( !m_modified) //don't save if no changes are made
			return;

		if ( oldfileversion)
		{
			if ( KMessageBox::warningContinueCancel( m_parent, i18n( "This file is saved with an old file format; if you save it, you cannot open the file with older versions of Kmplot. Are you sure you want to continue?" ), QString(), i18n("Save New Format") ) == KMessageBox::Cancel)
				return;
		}
		kmplotio->save( m_url );
		kDebug() << "saved" << endl;
		m_modified = false;
	}

}

void MainDlg::slotSaveas()
{
	if (m_readonly)
		return;
	const KUrl url = KFileDialog::getSaveURL( QDir::currentPath(), i18n( "*.fkt|KmPlot Files (*.fkt)\n*|All Files" ), m_parent, i18n( "Save As" ) );

	if ( !url.isEmpty() )
	{
		// check if file exists and overwriting is ok.
		if( !KIO::NetAccess::exists( url,false,m_parent ) || KMessageBox::warningContinueCancel( m_parent, i18n( "A file named \"%1\" already exists. Are you sure you want to continue and overwrite this file?", url.url()), i18n( "Overwrite File?" ), KGuiItem( i18n( "&Overwrite" ) ) ) == KMessageBox::Continue )
		{
			if ( !kmplotio->save( url ) )
				KMessageBox::error(m_parent, i18n("The file could not be saved") );
			else
			{
				m_url = url;
				m_recentFiles->addUrl( url );
				setWindowCaption( m_url.prettyUrl(KUrl::LeaveTrailingSlash) );
				m_modified = false;
			}
			return;
		}
	}
}

void MainDlg::slotExport()
{
	KUrl const url = KFileDialog::getSaveURL(QDir::currentPath(),
	                 i18n("*.svg|Scalable Vector Graphics (*.svg)\n"
	                      "*.bmp|Bitmap 180dpi (*.bmp)\n"
	                      "*.png|Bitmap 180dpi (*.png)"), m_parent, i18n("Export") );
	if( url.isEmpty() )
		return;
	
	// check if file exists and overwriting is ok.
	bool exists = KIO::NetAccess::exists(url,false,m_parent );
	if ( exists )
	{
		int result = KMessageBox::warningContinueCancel( m_parent, i18n( "A file named \"%1\" already exists. Are you sure you want to continue and overwrite this file?" , url.url() ), i18n( "Overwrite File?" ), KGuiItem( i18n( "&Overwrite" ) ) );
		if ( result != KMessageBox::Continue )
			return;
	}

	if( url.fileName().right(4).toLower()==".svg")
	{
		QPicture pic;
		View::self()->draw(&pic, View::SVG);
		if (url.isLocalFile() )
			pic.save( url.path(), "SVG");
		else
		{
			KTempFile tmp;
			pic.save( tmp.name(), "SVG");
			if ( !KIO::NetAccess::upload(tmp.name(), url, 0) )
				KMessageBox::error(m_parent, i18n("The URL could not be saved.") );
			tmp.unlink();
		}
	}

	else if( url.fileName().right(4).toLower()==".bmp")
	{
		QPixmap pic(100, 100);
		View::self()->draw(&pic, View::Pixmap);
		if (url.isLocalFile() )
			pic.save(  url.path(), "BMP");
		else
		{
			KTempFile tmp;
			pic.save( tmp.name(), "BMP");
			if ( !KIO::NetAccess::upload(tmp.name(), url, 0) )
				KMessageBox::error(m_parent, i18n("The URL could not be saved.") );
			tmp.unlink();
		}
	}

	else if( url.fileName().right(4).toLower()==".png")
	{
		QPixmap pic(100, 100);
		View::self()->draw(&pic, View::Pixmap);
		if (url.isLocalFile() )
			pic.save( url.path(), "PNG");
		else
		{
			KTempFile tmp;
			pic.save( tmp.name(), "PNG");
			if ( !KIO::NetAccess::upload(tmp.name(), url, 0) )
				KMessageBox::error(m_parent, i18n("The URL could not be saved.") );
			tmp.unlink();
		}
	}
}
bool MainDlg::openFile()
{
	View::self()->init();
	if (m_url==m_currentfile || !kmplotio->load( m_url ) )
	{
		m_recentFiles->removeUrl(m_url ); //remove the file from the recent-opened-file-list
		m_url = "";
		return false;
	}
	m_currentfile = m_url;
	m_recentFiles->addUrl( m_url.prettyUrl(KUrl::LeaveTrailingSlash)  );
	setWindowCaption( m_url.prettyUrl(KUrl::LeaveTrailingSlash) );
	m_modified = false;
	View::self()->updateSliders();
	View::self()->drawPlot();
	return true;
}

void MainDlg::slotOpenRecent( const KUrl &url )
{
	if( isModified() || !m_url.isEmpty() ) // open the file in a new window
	{
		QByteArray data;
		QDataStream stream( &data,QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_3_1);
		stream << url;
		KApplication::kApplication()->dcopClient()->send(KApplication::kApplication()->dcopClient()->appId(), "KmPlotShell","openFileInNewWindow(KUrl)", data);
		return;
	}

	View::self()->init();
	if ( !kmplotio->load( url ) ) //if the loading fails
	{
		m_recentFiles->removeUrl(url ); //remove the file from the recent-opened-file-list
		return;
	}
  m_url = m_currentfile = url;
	m_recentFiles->setCurrentItem(-1); //don't select the item in the open-recent menu
  setWindowCaption( m_url.prettyUrl(KUrl::LeaveTrailingSlash) );
  m_modified = false;
	View::self()->updateSliders();
	View::self()->drawPlot();
}

void MainDlg::slotPrint()
{
	KPrinter prt( QPrinter::PrinterResolution );
	prt.setResolution( 72 );
	KPrinterDlg* printdlg = new KPrinterDlg( m_parent );
	printdlg->setObjectName( "KmPlot page" );
	prt.addDialogPage( printdlg );
	if ( prt.setup( m_parent, i18n( "Print Plot" ) ) )
	{
		prt.setFullPage( true );
		View::self()->draw(&prt, View::Printer);
	}
}

void MainDlg::editAxes()
{
	// create a config dialog and add a axes page
	if ( !coordsDialog)
	{
		coordsDialog = new CoordsConfigDialog( XParser::self(), m_parent);
		// User edited the configuration - update your local copies of the
		// configuration data
		connect( coordsDialog, SIGNAL( settingsChanged(const QString &) ), this, SLOT(updateSettings() ) );
	}
	coordsDialog->show();
}

void MainDlg::editScaling()
{
	// create a config dialog and add a scaling page
	KConfigDialog *scalingDialog = new KConfigDialog( m_parent, "scaling", Settings::self() );
	scalingDialog->setHelp("scaling-config");
	EditScaling *es = new EditScaling();
	es->setObjectName( "scalingSettings" );
	scalingDialog->addPage( es, i18n( "Scale" ), "scaling", i18n( "Edit Scaling" ) );
	// User edited the configuration - update your local copies of the
	// configuration data
	connect( scalingDialog, SIGNAL( settingsChanged(const QString &) ), this, SLOT(updateSettings() ) );
	scalingDialog->show();
}

void MainDlg::slotNames()
{
	KToolInvocation::invokeHelp( "func-predefined", "kmplot" );
}


void MainDlg::slotCoord1()
{
	Settings::setXRange( 0 );
	Settings::setYRange( 0 );
	m_modified = true;
	View::self()->getSettings();
	View::self()->drawPlot();
}

void MainDlg::slotCoord2()
{
	Settings::setXRange( 2 );
	Settings::setYRange( 0 );
	m_modified = true;
	View::self()->getSettings();
	View::self()->drawPlot();
}

void MainDlg::slotCoord3()
{
	Settings::setXRange( 2 );
	Settings::setYRange( 2 );
	m_modified = true;
	View::self()->getSettings();
	View::self()->drawPlot();
}

void MainDlg::slotSettings()
{
	// An instance of your dialog has already been created and has been cached,
	// so we want to display the cached dialog instead of creating
	// another one
	KConfigDialog::showDialog( "settings" );
}

void MainDlg::updateSettings()
{
	View::self()->getSettings();
	m_modified = true;
	View::self()->drawPlot();
}


void MainDlg::getYValue()
{
	minmaxdlg->init( KMinMax::CalculateY );
	minmaxdlg->show();
}

void MainDlg::findMinimumValue()
{
	minmaxdlg->init( KMinMax::FindMinimum );
	minmaxdlg->show();
}

void MainDlg::findMaximumValue()
{
	minmaxdlg->init( KMinMax::FindMaximum );
	minmaxdlg->show();
}

void MainDlg::graphArea()
{
	minmaxdlg->init( KMinMax::CalculateArea );
	minmaxdlg->show();
}

void MainDlg::toggleShowSliders()
{
	// create the slider if it not exists already
	if ( !View::self()->m_sliderWindow )
	{
		View::self()->m_sliderWindow = new KSliderWindow( View::self(), actionCollection() );
		connect( View::self()->m_sliderWindow, SIGNAL( valueChanged() ), View::self(), SLOT( drawPlot() ) );
		connect( View::self()->m_sliderWindow, SIGNAL( windowClosed() ), View::self(), SLOT( slidersWindowClosed() ) );
	}
	if ( !View::self()->m_sliderWindow->isVisible() )
		View::self()->m_sliderWindow->show();
	else
		View::self()->m_sliderWindow->hide();
}

void MainDlg::setReadOnlyStatusBarText(const QString &text)
{
	setStatusBarText(text);
}

void MainDlg::optionsConfigureKeys()
{
	KApplication::kApplication()->dcopClient()->send(KApplication::kApplication()->dcopClient()->appId(), "KmPlotShell","optionsConfigureKeys()", QByteArray());
}

void MainDlg::optionsConfigureToolbars()
{
	KApplication::kApplication()->dcopClient()->send(KApplication::kApplication()->dcopClient()->appId(), "KmPlotShell","optionsConfigureToolbars()", QByteArray());
}

// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>
#include <klocale.h>
#include <ktoolinvocation.h>
#include <kglobal.h>

KInstance*  KmPlotPartFactory::s_instance = 0L;
KAboutData* KmPlotPartFactory::s_about = 0L;

KmPlotPartFactory::KmPlotPartFactory()
		: KParts::Factory()
{}

KmPlotPartFactory::~KmPlotPartFactory()
{
	delete s_instance;
	delete s_about;

	s_instance = 0L;
}

KParts::Part* KmPlotPartFactory::createPartObject( QWidget *parentWidget,
        QObject *parent, const char *, const QStringList &args )
{
	// Create an instance of our Part
	MainDlg* obj = new MainDlg( parentWidget, parent, args );
	emit objectCreated( obj );
	return obj;
}

KInstance* KmPlotPartFactory::instance()
{
	if( !s_instance )
	{
		s_about = new KAboutData("kmplot",I18N_NOOP( "KmPlotPart" ), "1");
		s_instance = new KInstance(s_about);
	}
	return s_instance;
}

extern "C"
{
	KDE_EXPORT void* init_libkmplotpart()
	{
		return new KmPlotPartFactory;
	}
}


/// BrowserExtension class
BrowserExtension::BrowserExtension(MainDlg* parent)
		: KParts::BrowserExtension( parent )
{
	emit enableAction("print", true);
	setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
	static_cast<MainDlg*>(parent())->slotPrint();
}

