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
     * Simple popup dialog to show a message just prior to a lengthy
     * operation. The popup does some event handling to make sure that it
     * really appears before that lengthy operation starts: it spins an
     * event loop until its own show, resize, move and paint events have
     * been processed.
     *
     * Usage:
     *
     *	   BusyPopup msg( "Calculating...", mainWin );
     *	   longCalculation();
     *
     * In the normal case, just let the variable go out of scope and the
     * popup is destroyed and thus closed.
     **/
    class BusyPopup final : public QLabel
    {
	Q_OBJECT

    public:

	/**
	 * Create a BusyPopup with the specified text.  This always uses
	 * the main window as its parent.
	 *
	 * Show the label and spin an event loop unitl the label has been
	 * painted.
	 **/
	BusyPopup( const QString & text, QWidget * parent = nullptr );

	/**
	 * Event handler. Reimplemented from QWidget.
	 *
	 * Show event: position the popup in the center of the parent window.
	 *
	 * Paint event: emit a signal that the popup is (most probably) now
	 * visible.  The signal is actually emitted before the label is
	 * painted, but it will be painted before the event handler returns.
	 **/
	bool event( QEvent * event ) override;


    signals:

	void painted();

    };	// class BusyPopup

}	// namespace QDirStat

#endif	// BusyPopup_h
