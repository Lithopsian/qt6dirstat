/*
 *   File name: BusyPopup.h
 *   Summary:   QDirStat generic widget classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef BusyPopup_h
#define BusyPopup_h

#include <QLabel>


namespace QDirStat
{
    /**
     * Simple popup dialog to show a message just prior to a lengthy operation.
     * The popup does some event handling to make sure that it really appears
     * before that lengthy operation starts: it processes events for some
     * milliseconds so that its own show, resize, move and paint events are
     * processed.
     *
     * Usage:
     *
     *	   BusyPopup msg( "Calculating...", mainWin );
     *	   calc();
     *
     * In the normal case, just let the variable go out of scope, and the popup
     * is destroyed and thus closed. Of course you can also create it with
     * 'new' and destroy it with 'delete' or simply call 'hide()'.
     **/
    class BusyPopup: public QLabel
    {
	Q_OBJECT

    public:

	/**
	 * Create a BusyPopup with the specified text.  This always uses
	 * the main window as its parent.
	 *
	 * Show the label and process events for some milliseconds to makes
	 * sure it is rendered.
	 **/
	BusyPopup( const QString & text );

	/**
	 * Process a show event.  Positions the popup once it has been shown.
	 *
	 * Reimplemented from QLabel / QWidget.
	 **/
	void showEvent( QShowEvent * event ) override;

    };	// class BusyPopup

}	// namespace QDirStat

#endif	// BusyPopup_h
