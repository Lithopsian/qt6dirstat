/*
 *   File name: PanelMessage.h
 *   Summary:   Message in a panel with icon and close button
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef PanelMessage_h
#define PanelMessage_h

#include <memory>

#include <QWidget>

#include "ui_panel-message.h"


namespace QDirStat
{
    class MainWindow;

    /**
     * Message in a small panel with an icon, a bold face heading,
     * a message text, an optional "Details..." hyperlink
     * and a small [x] window close button.
     *
     * The close button calls deleteLater on the panel, so it is completely
     * self-sufficient once set up.
     **/
    class PanelMessage final : public QWidget
    {
	Q_OBJECT

	/**
	 * Constructor.  Private, use the static methods for access.
	 **/
	PanelMessage();


    public:

	/**
	 * Delete any panel message child of 'parent' that is currently
	 * displaying a permissions message.
	 **/
	static void deletePermissionsMsg( const QWidget * parent );

	/**
	 * Show the particular type of panel message in the given container.  These wrappers
	 * maintain an internal static smart pointer to the panel so that only one panel
	 * of each type is created.
	 **/
	static void showPermissionsMsg( MainWindow * mainWin, QVBoxLayout * vBox );
	static void showFilesystemsMsg( QVBoxLayout * vBox );
	static void showRpmMsg( MainWindow * mainWin, QVBoxLayout * vBox );


    protected:

	/**
	 * Show a panel message in the given container.
	 **/
	static PanelMessage * createMsg( QVBoxLayout * vBox );


    private:

	std::unique_ptr<Ui::PanelMessage> _ui;

    };	// class PanelMessage

}	// namespace QDirStat

#endif	// PanelMessage_h
