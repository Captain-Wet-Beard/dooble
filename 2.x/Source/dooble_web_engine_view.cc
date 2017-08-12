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

#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_page.h"
#include "dooble_web_engine_url_request_interceptor.h"
#include "dooble_web_engine_view.h"

dooble_web_engine_view::dooble_web_engine_view(bool is_private,
					       QWidget *parent):
  QWebEngineView(parent)
{
  m_cookies = 0;
  m_is_private = is_private;

  if(m_is_private)
    m_page = new dooble_web_engine_page
      (new QWebEngineProfile(this), true, this);
  else
    m_page = new dooble_web_engine_page
      (QWebEngineProfile::defaultProfile(), false, this);

  if(m_is_private)
    {
      m_cookies = new dooble_cookies(m_is_private, this);
      m_cookies_window = new dooble_cookies_window(m_is_private, this);
      m_page->profile()->setRequestInterceptor
	(dooble::s_url_request_interceptor);
      connect(m_cookies,
	      SIGNAL(cookie_added(const QNetworkCookie &, bool)),
	      m_cookies_window,
	      SLOT(slot_cookie_added(const QNetworkCookie &, bool)));
      connect(m_cookies,
	      SIGNAL(cookie_removed(const QNetworkCookie &)),
	      m_cookies_window,
	      SLOT(slot_cookie_removed(const QNetworkCookie &)));
      connect(m_page->profile()->cookieStore(),
	      SIGNAL(cookieAdded(const QNetworkCookie &)),
	      m_cookies,
	      SLOT(slot_cookie_added(const QNetworkCookie &)));
      connect(m_page->profile()->cookieStore(),
	      SIGNAL(cookieRemoved(const QNetworkCookie &)),
	      m_cookies,
	      SLOT(slot_cookie_removed(const QNetworkCookie &)));
    }
}

dooble_web_engine_view *dooble_web_engine_view::createWindow
(QWebEnginePage::WebWindowType type)
{
  dooble_web_engine_view *view = new dooble_web_engine_view(m_is_private, 0);

  switch(type)
    {
    case QWebEnginePage::WebBrowserWindow:
    case QWebEnginePage::WebDialog:
      {
	emit create_window(view);
	break;
      }
    default:
      emit create_tab(view);
    }

  return view;
}

void dooble_web_engine_view::show_private_cookies(void)
{
  if(!m_cookies_window)
    return;

  m_cookies_window->filter(url().host());
  m_cookies_window->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, m_cookies_window);

  m_cookies_window->activateWindow();
  m_cookies_window->raise();
}
