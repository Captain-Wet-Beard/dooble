/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QClipboard>
#include <QFileDialog>
#include <QKeyEvent>
#include <QShortcut>
#include <QStandardPaths>
#include <QWebEngineDownloadItem>

#include "dooble.h"
#include "dooble_cryptography.h"
#include "dooble_downloads.h"
#include "dooble_downloads_item.h"
#include "dooble_settings.h"

dooble_downloads::dooble_downloads(void):QMainWindow()
{
  m_download_path_inspection_timer.start(2500);
  m_ui.setupUi(this);
  m_ui.download_path->setText
    (dooble_settings::setting("download_path").toString());

  if(m_ui.download_path->text().isEmpty())
    m_ui.download_path->setText
      (QStandardPaths::
       standardLocations(QStandardPaths::DesktopLocation).value(0));

  connect(&m_download_path_inspection_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_download_path_inspection_timer_timeout(void)));
  connect(m_ui.clear_finished_downloads,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_clear_finished_downloads(void)));
  connect(m_ui.select,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_select_path(void)));
  connect(m_ui.table,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
  new QShortcut // Without a parent.
    (QKeySequence(tr("Ctrl+F")), m_ui.search, SLOT(setFocus(void)));
  m_ui.table->setContextMenuPolicy(Qt::CustomContextMenu);
}

QString dooble_downloads::download_path(void) const
{
  return m_ui.download_path->text();
}

bool dooble_downloads::contains(QWebEngineDownloadItem *download) const
{
  return m_downloads.contains(download);
}

void dooble_downloads::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_downloads::delete_selected(void)
{
  QModelIndexList list(m_ui.table->selectionModel()->selectedIndexes());

  for(int i = list.size() - 1; i >= 0; i--)
    {
      dooble_downloads_item *downloads_item = qobject_cast
	<dooble_downloads_item *> (m_ui.table->cellWidget(list.at(i).row(), 0));

      if(!downloads_item)
	continue;
      else if(!downloads_item->is_finished())
	continue;

      m_ui.table->removeRow(list.at(i).row());
      downloads_item->deleteLater();
    }
}

void dooble_downloads::keyPressEvent(QKeyEvent *event)
{
  if(event)
    {
      if(event->key() == Qt::Key_Delete)
	delete_selected();
      else if(event->key() == Qt::Key_Escape)
	close();
    }

  QMainWindow::keyPressEvent(event);
}

void dooble_downloads::populate(void)
{
}

void dooble_downloads::record_download(QWebEngineDownloadItem *download)
{
  if(!download)
    return;
  else if(m_downloads.contains(download))
    return;
  else
    m_downloads[download] = 0;

  connect(download,
	  SIGNAL(destroyed(void)),
	  this,
	  SLOT(slot_download_destroyed(void)));

  dooble_downloads_item *downloads_item = new dooble_downloads_item
    (download, this);

  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setCellWidget(m_ui.table->rowCount() - 1, 0, downloads_item);
  m_ui.table->resizeRowToContents(m_ui.table->rowCount() - 1);
}

void dooble_downloads::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_downloads::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("downloads_geometry", saveGeometry().toBase64());
}

void dooble_downloads::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::setting("downloads_geometry").
			      toByteArray()));

  QMainWindow::show();
  populate();
}

void dooble_downloads::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::setting("downloads_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
  populate();
}

void dooble_downloads::slot_clear_finished_downloads(void)
{
  delete_selected();
}

void dooble_downloads::slot_copy_download_location(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QClipboard *clipboard = QApplication::clipboard();

  if(clipboard)
    clipboard->setText(action->property("url").toUrl().toString());
}

void dooble_downloads::slot_delete_row(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  dooble_downloads_item *downloads_item = qobject_cast<dooble_downloads_item *>
    (m_ui.table->cellWidget(action->property("row").toInt(), 0));

  if(downloads_item && downloads_item->is_finished())
    {
      downloads_item->deleteLater();
      m_ui.table->removeRow(action->property("row").toInt());
    }
}

void dooble_downloads::slot_download_destroyed(void)
{
  m_downloads.remove(sender());
}

void dooble_downloads::slot_download_path_inspection_timer_timeout(void)
{
  QFileInfo file_info(m_ui.download_path->text());
  QPalette palette(m_ui.download_path->palette());
  static QPalette s_palette(m_ui.download_path->palette());

  if(file_info.isWritable())
    palette = s_palette;
  else
    palette.setColor
      (m_ui.download_path->backgroundRole(), QColor(240, 128, 128));

  m_ui.download_path->setPalette(palette);
}

void dooble_downloads::slot_open_download_page(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QUrl url(action->property("url").toUrl().adjusted(QUrl::RemoveFilename));

  if(url.isEmpty() || !url.isValid())
    return;

  /*
  ** Locate a Dooble window.
  */

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  disconnect(this,
	     SIGNAL(open_url(const QUrl &)));

  QWidgetList list(QApplication::topLevelWidgets());

  for(int i = 0; i < list.size(); i++)
    if(qobject_cast<dooble *> (list.at(i)) &&
       qobject_cast<dooble *> (list.at(i))->isVisible())
      {
	connect(this,
		SIGNAL(open_url(const QUrl &)),
		list.at(i),
		SLOT(slot_open_url(const QUrl &)),
		Qt::UniqueConnection);
	break;
      }

  QApplication::restoreOverrideCursor();
  emit open_url(url);
}

void dooble_downloads::slot_select_path(void)
{
  QFileDialog dialog(this);

  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setWindowTitle(tr("Dooble: Select Download Path"));

  if(dialog.exec() == QDialog::Accepted)
    {
      dooble_settings::set_setting
	("download_path", dialog.selectedFiles().value(0));
      m_ui.download_path->setText(dialog.selectedFiles().value(0));
    }
}

void dooble_downloads::slot_show_context_menu(const QPoint &point)
{
  int row = m_ui.table->rowAt(point.y());

  if(row < 0)
    return;

  dooble_downloads_item *downloads_item = qobject_cast
    <dooble_downloads_item *> (m_ui.table->cellWidget(row, 0));

  if(!downloads_item)
    return;

  QAction *action = 0;
  QMenu menu(this);
  QUrl url(downloads_item->url());

  action = menu.addAction
    (tr("&Copy Download Location"),
     this,
     SLOT(slot_copy_download_location(void)));
  action->setEnabled(!url.isEmpty() && url.isValid());
  action->setProperty("url", url);
  action = menu.addAction
    (tr("&Open Download Page"),
     this,
     SLOT(slot_open_download_page(void)));
  action->setEnabled(!url.isEmpty() && url.isValid());
  action->setProperty("url", url);
  menu.addSeparator();
  action = menu.addAction(tr("&Delete"), this, SLOT(slot_delete_row(void)));
  action->setEnabled(downloads_item->is_finished());
  action->setProperty("row", row);
  menu.exec(mapToGlobal(point));
}
