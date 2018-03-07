/*
 * DesktopSatelliteWindow.cpp
 *
 * Copyright (C) 2009-18 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "DesktopSatelliteWindow.hpp"

#include <QShortcut>

namespace rstudio {
namespace desktop {


SatelliteWindow::SatelliteWindow(MainWindow* pMainWindow, QString name) :
    GwtWindow(false, true, name),
    gwtCallback_(pMainWindow, this),
    close_(CloseStageOpen)
{
   setAttribute(Qt::WA_QuitOnClose, false);
   setAttribute(Qt::WA_DeleteOnClose, true);

   setWindowIcon(QIcon(QString::fromUtf8(":/icons/RStudio.ico")));

   // satellites don't have a menu, so connect zoom keyboard shortcuts
   // directly
   // NOTE: CTRL implies META on macOS
   QShortcut* zoomActualSizeShortcut = new QShortcut(Qt::CTRL + Qt::Key_0, this);
   QShortcut* zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
   QShortcut* zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
   
   connect(zoomActualSizeShortcut, SIGNAL(activated()), this, SLOT(zoomActualSize()));
   connect(zoomInShortcut, SIGNAL(activated()), this, SLOT(zoomIn()));
   connect(zoomOutShortcut, SIGNAL(activated()), this, SLOT(zoomOut()));
}

void SatelliteWindow::onCloseWindowShortcut()
{
   close();
}

void SatelliteWindow::finishLoading(bool ok)
{
   BrowserWindow::finishLoading(ok);

   if (ok)
   {
      avoidMoveCursorIfNecessary();
      connect(webView(), SIGNAL(onCloseWindowShortcut()), this,
              SLOT(onCloseWindowShortcut()));
   }
}

void SatelliteWindow::closeSatellite(QCloseEvent *event)
{
   webPage()->runJavaScript(
      QStringLiteral(
          "if (window.notifyRStudioSatelliteClosing) "
          "   window.notifyRStudioSatelliteClosing();"));
   webView()->event(event);
}

void SatelliteWindow::closeEvent(QCloseEvent *event)
{
   // the source window has special close semantics; if we're not currently
   // closing, then invoke custom close handlers
   if (getName().startsWith(QString::fromUtf8(SOURCE_WINDOW_PREFIX)) &&
       close_ == CloseStageOpen)
   {
      // ignore this event; we need to make sure the window can be
      // closed ourselves
      event->ignore();
      close_ = CloseStagePending;

      webPage()->runJavaScript(
               QStringLiteral("window.rstudioReadyToClose"),
               [&](QVariant qReadyToClose)
      {
         bool readyToClose = qReadyToClose.toBool();
         if (readyToClose)
         {
            // all clear, close the window
            close_ = CloseStageAccepted;
            close();
         }
         else
         {
            // not ready to close, revert close stage and take care of business
            close_ = CloseStageOpen;
            webPage()->runJavaScript(
                     QStringLiteral("window.rstudioCloseSourceWindow()"),
                     [&](QVariant ignored)
            {
               // no work to do here
            });
         }
      });
   }
   else
   {
      // not a  source window, just close it
      closeSatellite(event);
   }
}

void SatelliteWindow::onActivated()
{
   webView()->webPage()->runJavaScript(
            QString::fromUtf8(
               "if (window.notifyRStudioSatelliteReactivated) "
               "   window.notifyRStudioSatelliteReactivated(null);"));
}


} // namespace desktop
} // namespace rstudio
