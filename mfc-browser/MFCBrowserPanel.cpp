#include <obs-frontend-api.h>

#include <QDir>
#include <QThread>
#include <QMessageBox>
#include <QDockWidget>

#include "cef-headers.hpp"
#include <QScopedPointer>

#include <browser-panel.hpp>
#include <qt-wrappers.hpp>

extern "C" QCef *obs_browser_create_qcef(void);
std::map<std::string, QDockWidget *> m_mapDocks;

QCefCookieManager *panel_cookies;

extern "C"
{
	EXPORT void openBrowserPanel(const char *pCaption, const char *pURL, int nWidth, int nHeight)
	{
		void *pWindow = obs_frontend_get_main_window();

		std::string sObjectName = pCaption;
		sObjectName += "obj_name";
		std::replace(sObjectName.begin(), sObjectName.end(),
					' ', '_');
		int nMinWidth = 150;
		int nMinHeight = 150;
		std::map<std::string, QDockWidget *>::iterator itr =
			m_mapDocks.find(sObjectName);
		if (itr == m_mapDocks.end())
		{
			// opens generic panel.
			QDockWidget *pDock = new QDockWidget();
			QCef *pCef = obs_browser_create_qcef();

			const auto initPanelCookieManager = [&]()
			{
				if (!pCef) return;
				if (panel_cookies) return;
				std::string sub_path = "obs_profile_cookies/sidekick";
				panel_cookies = pCef->create_cookie_manager(sub_path);
			};

			if (pCef->init_browser())
			{
				initPanelCookieManager();
			}
			else
			{
				ExecThreadedWithoutBlocking([&] { pCef->wait_for_browser_init(); },
							    QString("BrowserPanelInit.Title"),
							    QString("BrowserPanelInit.Text"));
				initPanelCookieManager();
			}
			if (panel_cookies)
				panel_cookies->DeleteCookies("", "");

			QCefWidget *pBrowser = pCef->create_widget(pDock, pURL, panel_cookies);

			pBrowser->allowAllPopups(true);
			pDock->setWidget(pBrowser);

			pDock->setObjectName(sObjectName.c_str());
			pDock->setWindowTitle(pCaption);

			pDock->setFeatures(QDockWidget::AllDockWidgetFeatures); //QDock
			pDock->setMinimumSize(nMinWidth, nMinHeight);

			if (nWidth > 0 && nHeight > 0)
				pDock->resize(nWidth, nHeight);

			pDock->setAllowedAreas(Qt::AllDockWidgetAreas);

			QAction *pAction = (QAction *)obs_frontend_add_dock(pDock);
			pDock->setFloating(true);
			pDock->show();
			m_mapDocks[sObjectName] = pDock;
		}
		else
		{
			QDockWidget *pDock = itr->second;
			pDock->show();
		}

	}

	EXPORT void closeBrowserPanel(const char *pCaption)
	{
		std::string sObjectName = pCaption;
		sObjectName += "obj_name";
		std::replace(sObjectName.begin(), sObjectName.end(), ' ', '_');
		int nMinWidth = 150;
		int nMinHeight = 150;
		std::map<std::string, QDockWidget *>::iterator itr =
			m_mapDocks.find(sObjectName);
		if (itr != m_mapDocks.end())
		{
			QDockWidget *pDock = itr->second;
			pDock->hide();
		}
	}


	EXPORT void clearBrowserPanel()
	{
		std::map<std::string, QDockWidget *>::iterator itr =
			m_mapDocks.begin();
		while (itr != m_mapDocks.end())
		{
			QDockWidget *pDock = itr->second;
			pDock->hide();
			delete pDock;
		}
		m_mapDocks.clear();
	}
}
