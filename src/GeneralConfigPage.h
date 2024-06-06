/*
 *   File name: GeneralConfigPage.h
 *   Summary:   QDirStat configuration dialog classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef GeneralConfigPage_h
#define GeneralConfigPage_h

#include "ui_general-config-page.h"


namespace QDirStat
{
    class ConfigDialog;

    class GeneralConfigPage: public QWidget
    {
        Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
        GeneralConfigPage( ConfigDialog * parent );


    protected slots:

	/**
	 * Apply changes to the settings.  The values are sent directly to MainWindow
	 * and DirTreeModel, the two classes that use the settings on this page.
	 **/
	void applyChanges();


    protected:

	/**
	 * Populate the widgets from the values held in MainWindow and DirTreeModel.
	 **/
	void setup();


    private:

	//
	// Data members
	//

	std::unique_ptr<Ui::GeneralConfigPage> _ui;

    }; // class GeneralConfigPage
}

#endif // GeneralConfigPage_h
