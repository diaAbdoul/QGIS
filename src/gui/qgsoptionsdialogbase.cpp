/***************************************************************************
    qgsoptionsdialogbase.cpp - base vertical tabs option dialog

    ---------------------
    begin                : March 24, 2013
    copyright            : (C) 2013 by Larry Shaffer
    email                : larrys at dakcarto dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsoptionsdialogbase.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QStackedWidget>
#include <QSplitter>
#include <QTimer>


QgsOptionsDialogBase::QgsOptionsDialogBase( QString settingsKey, QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl ), mOptsKey( settingsKey ), mInit( false )
{
}

QgsOptionsDialogBase::~QgsOptionsDialogBase()
{
  if ( mInit )
  {
    QSettings settings;
    settings.setValue( QString( "/Windows/%1/geometry" ).arg( mOptsKey ), saveGeometry() );
    settings.setValue( QString( "/Windows/%1/splitState" ).arg( mOptsKey ), mOptSplitter->saveState() );
    settings.setValue( QString( "/Windows/%1/tab" ).arg( mOptsKey ), mOptStackedWidget->currentIndex() );
  }
}

void QgsOptionsDialogBase::initOptionsBase( bool restoreUi )
{
  // don't add to dialog margins
  // redefine now, or those in inherited .ui file will be added
  if ( layout() )
    layout()->setContentsMargins( 12, 12, 12, 12 ); // Qt default spacing

  // start with copy of qgsoptionsdialog_template.ui to ensure existence of these objects
  mOptListWidget = findChild<QListWidget*>( "mOptionsListWidget" );
  mOptStackedWidget = findChild<QStackedWidget*>( "mOptionsStackedWidget" );
  mOptSplitter = findChild<QSplitter*>( "mOptionsSplitter" );
  mOptButtonBox = findChild<QDialogButtonBox*>( "buttonBox" );

  if ( !mOptListWidget || !mOptStackedWidget || !mOptSplitter )
  {
    return;
  }

  if ( mOptButtonBox )
  {
    // enforce only one connection per signal, in case added in Qt Designer
    disconnect( mOptButtonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    connect( mOptButtonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    disconnect( mOptButtonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
    connect( mOptButtonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  }
  connect( mOptSplitter, SIGNAL( splitterMoved( int, int ) ), this, SLOT( updateOptionsListVerticalTabs() ) );
  connect( mOptStackedWidget, SIGNAL( currentChanged( int ) ), this, SLOT( optionsStackedWidget_CurrentChanged( int ) ) );
  connect( mOptStackedWidget, SIGNAL( widgetRemoved( int ) ), this, SLOT( optionsStackedWidget_WidgetRemoved( int ) ) );

  if ( restoreUi )
    restoreOptionsBaseUi();

  mInit = true;
}

void QgsOptionsDialogBase::restoreOptionsBaseUi()
{
  if ( !mInit )
  {
    return;
  }

  QSettings settings;
  restoreGeometry( settings.value( QString( "/Windows/%1/geometry" ).arg( mOptsKey ) ).toByteArray() );
  // mOptListWidget width is fixed to take up less space in QtDesigner
  // revert it now unless the splitter's state hasn't been saved yet
  mOptListWidget->setMaximumWidth(
    settings.value( QString( "/Windows/%1/splitState" ).arg( mOptsKey ) ).isNull() ? 150 : 16777215 );
  mOptSplitter->restoreState( settings.value( QString( "/Windows/%1/splitState" ).arg( mOptsKey ) ).toByteArray() );
  int curIndx = settings.value( QString( "/Windows/%1/tab" ).arg( mOptsKey ), 0 ).toInt();

  // if the last used tab is not enabled, or is missing, display the first enabled one
  if ( !mOptStackedWidget->widget( curIndx )->isEnabled()
       || mOptStackedWidget->count() < ( curIndx + 1 ) )
  {
    curIndx = 0;
    for ( int i = 0; i < mOptStackedWidget->count(); i++ )
    {
      if ( mOptStackedWidget->widget( i )->isEnabled() )
      {
        curIndx = i;
        break;
      }
    }
    curIndx = -1; // default fallback
  }

  mOptStackedWidget->setCurrentIndex( curIndx );
  mOptListWidget->setCurrentRow( curIndx );

  // get rid of annoying outer focus rect on Mac
  mOptListWidget->setAttribute( Qt::WA_MacShowFocusRect, false );
}

void QgsOptionsDialogBase::showEvent( QShowEvent* e )
{
  if ( mInit )
  {
    updateOptionsListVerticalTabs();
  }
  else
  {
    QTimer::singleShot( 0, this, SLOT( warnAboutMissingObjects() ) );
  }

  QDialog::showEvent( e );
}

void QgsOptionsDialogBase::paintEvent( QPaintEvent* e )
{
  if ( mInit )
    QTimer::singleShot( 0, this, SLOT( updateOptionsListVerticalTabs() ) );

  QDialog::paintEvent( e );
}

void QgsOptionsDialogBase::updateOptionsListVerticalTabs()
{
  if ( !mInit )
    return;

  if ( mOptListWidget->maximumWidth() != 16777215 )
    mOptListWidget->setMaximumWidth( 16777215 );
  // auto-resize splitter for vert scrollbar without covering icons in icon-only mode
  // TODO: mOptListWidget has fixed 32px wide icons for now, allow user-defined
  // Note: called on splitter resize and dialog paint event, so only update when necessary
  int iconWidth = mOptListWidget->iconSize().width();
  int snapToIconWidth = iconWidth + 32;

  QList<int> splitSizes = mOptSplitter->sizes();
  bool iconOnly = ( splitSizes.at( 0 ) <= snapToIconWidth );

  int newWidth = mOptListWidget->verticalScrollBar()->isVisible() ? iconWidth + 26 : iconWidth + 12;
  bool diffWidth = mOptListWidget->minimumWidth() != newWidth;

  if ( diffWidth )
    mOptListWidget->setMinimumWidth( newWidth );

  if ( iconOnly && ( diffWidth || mOptListWidget->width() != newWidth ) )
  {
    splitSizes[1] = splitSizes.at( 1 ) - ( splitSizes.at( 0 ) - newWidth );
    splitSizes[0] = newWidth;
    mOptSplitter->setSizes( splitSizes );
  }
  if ( mOptListWidget->wordWrap() && iconOnly )
    mOptListWidget->setWordWrap( false );
  if ( !mOptListWidget->wordWrap() && !iconOnly )
    mOptListWidget->setWordWrap( true );
}

void QgsOptionsDialogBase::optionsStackedWidget_CurrentChanged( int indx )
{
  mOptListWidget->blockSignals( true );
  mOptListWidget->setCurrentRow( indx );
  mOptListWidget->blockSignals( false );
}

void QgsOptionsDialogBase::optionsStackedWidget_WidgetRemoved( int indx )
{
  // will need to take item first, if widgets are set for item in future
  delete mOptListWidget->item( indx );
}

void QgsOptionsDialogBase::warnAboutMissingObjects()
{
  QMessageBox::warning( 0, tr( "Missing objects" ),
                        tr( "Base options dialog could not be initialized.\n\n"
                            "Missing some of the .ui template objects:\n" )
                        + " mOptionsListWidget,\n mOptionsStackedWidget,\n mOptionsSplitter",
                        QMessageBox::Ok,
                        QMessageBox::Ok );
}