/*
 *   File name: PanelMessage.h
 *   Summary:	Message in a panel with icon and close button
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */

#ifndef PanelMessage_h
#define PanelMessage_h

#include <QWidget>

#include "ui_panel-message.h"


namespace QDirStat
{
    class PanelMessage;

    /**
     * Message in a small panel with an icon, a bold face heading,
     * a message text, an optional "Details..." hyperlink
     * and a small [x] window close button.
     *
     * The close button calls deleteLater on the panel, so it is completely
     * self-sufficient once set up.
     **/
    class PanelMessage: public QWidget
    {
	Q_OBJECT

    protected:
	/**
	 * Constructor.  Private, use the static methods for access.
	 **/
	PanelMessage( QWidget * parent );

    public:
	/**
	 * Constructor.
	 **/
//	PanelMessage( QWidget		* parent,
//		      const QString	& heading,
//		      const QString	& msg );

	/**
	 * Destructor.
	 **/
	~PanelMessage() override;

	/**
	 * Set the "Details..." link text.
	 **/
//	void setDetails( const QString & urlText );

	/**
	 * Connect the "Details..." hyperlink to a receiver's slot.
	 * The hyperlink is only shown if it is connected.
	 *
	 * Use the same Qt macros as if connecting a normal widget:
	 *
	 *   connectDetailsLink( someAction, SLOT( triggered() ) );
	 **/
//	void connectDetailsLink( const QObject * receiver,
//				 const char    * slotName );

	/**
	 * Connect the "Details..." hyperlink to a web URL that will be opened
	 * in an external browser.
	 **/
//	void setDetailsUrl( const QString url );

	/**
	 * Return the URL set with setDetailsUrl().
	 **/
	const QLabel * detailsLinkLabel() const { return _ui->detailsLinkLabel; }

	/**
	 * Set the icon. If not set, a generic light bulb icon is used.
	 **/
//	void setIcon( const QPixmap & pixmap );

	/**
	 * Show a panel message in the given container.
	 **/
	static PanelMessage * createMsg( QWidget * parent, QVBoxLayout * vBox );
	static QPointer<PanelMessage> showPermissionsMsg( QWidget * parent, QVBoxLayout * vBox );
	static QPointer<PanelMessage> showFilesystemsMsg( QWidget * parent, QVBoxLayout * vBox );
	static QPointer<PanelMessage> showRpmMsg( QWidget * parent, QVBoxLayout * vBox );



    private:

	Ui::PanelMessage * _ui;

    };	// class PanelMessage

}	// namespace QDirStat

#endif	// PanelMessage_h
