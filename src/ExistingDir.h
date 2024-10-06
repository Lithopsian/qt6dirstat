/*
 *   File name: ExistingDir.h
 *   Summary:   QDirStat widget support classes
 *   License:   GPL V2 - See file LICENSE for details.
 *
 *   Authors:   Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 *              Ian Nartowicz
 */

#ifndef ExistingDir_h
#define ExistingDir_h

#include <QValidator>
#include <QCompleter>


namespace QDirStat
{
    /**
     * Validator class for QCombobox and related to validate names of existing
     * directories.
     *
     * See OpenUnpkgDialog for a usage example.
     **/
    class ExistingDirValidator: public QValidator
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	ExistingDirValidator( QObject * parent ):
	    QValidator{ parent }
	{}

	/**
	 * Validate the input string to see if it represents an
	 * existing directory.
	 **/
	QValidator::State validate( QString & input, int & ) const override;


    signals:

	void isOk( bool ok ) const;

    };	// class ExistingDirValidator


    /**
     * Completer class for QCombobox and related to complete names of existing
     * directories.
     *
     * See OpenUnpkgDialog for a usage example.
     **/
    class ExistingDirCompleter: public QCompleter
    {
	Q_OBJECT

    public:

	/**
	 * Constructor.
	 **/
	ExistingDirCompleter( QObject * parent );

    };	// class ExistingDirCompleter

}	// namespace QDirStat

#endif	// ExistingDir_h
