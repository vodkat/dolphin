/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qdir.h>

#include <klocale.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <krun.h>
#include <kprotocolinfo.h>
#include <kiconloader.h>
#include <klineeditdlg.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <kxmlguibuilder.h>
#include <kparts/componentfactory.h>

#include <assert.h>

#include <kfileshare.h>
#include <kprocess.h>
#include <libintl.h>

#include "kpropertiesdialog.h"
#include "knewmenu.h"
#include "konq_popupmenu.h"
#include "konq_operations.h"

class KonqPopupMenuGUIBuilder : public KXMLGUIBuilder
{
public:
  KonqPopupMenuGUIBuilder( QPopupMenu *menu )
  : KXMLGUIBuilder( 0 )
  {
    m_menu = menu;
  }
  virtual ~KonqPopupMenuGUIBuilder()
  {
  }

  virtual QWidget *createContainer( QWidget *parent, int index,
          const QDomElement &element,
          int &id )
  {
    if ( !parent && element.attribute( "name" ) == "popupmenu" )
      return m_menu;

    return KXMLGUIBuilder::createContainer( parent, index, element, id );
  }

  QPopupMenu *m_menu;
};

class KonqPopupMenu::KonqPopupMenuPrivate
{
public:
  KonqPopupMenuPrivate() : m_parentWidget(0) {}
  QString m_urlTitle;
  QWidget *m_parentWidget;
};

KonqPopupMenu::ProtocolInfo::ProtocolInfo( )
{
  m_Reading = false;
  m_Writing = false;
  m_Deleting = false;
  m_Moving = false;
  m_TrashIncluded = false;
}
bool KonqPopupMenu::ProtocolInfo::supportsReading() const
{
  return m_Reading;
}
bool KonqPopupMenu::ProtocolInfo::supportsWriting() const
{
  return m_Writing;
}
bool KonqPopupMenu::ProtocolInfo::supportsDeleting() const
{
  return m_Deleting;
}
bool KonqPopupMenu::ProtocolInfo::supportsMoving() const
{
  return m_Moving;
}
bool KonqPopupMenu::ProtocolInfo::trashIncluded() const
{
  return m_TrashIncluded;
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
                  bool showPropertiesAndFileType )
  : QPopupMenu( 0L, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QObject *>( 0 ), "KonqPopupMenu::m_ownActions" ),
    m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
  d = new KonqPopupMenuPrivate;
  setup(showPropertiesAndFileType);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              KActionCollection & actions,
                              KNewMenu * newMenu,
			      QWidget * parentWidget,
                  bool showPropertiesAndFileType )
  : QPopupMenu( 0L, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<QObject *>( 0 ), "KonqPopupMenu::m_ownActions" ),
    m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
  d = new KonqPopupMenuPrivate;
  d->m_parentWidget = parentWidget;
  setup(showPropertiesAndFileType);
}

void KonqPopupMenu::setup(bool showPropertiesAndFileType)
{
  assert( m_lstItems.count() >= 1 );

  m_ownActions.setWidget( this );

  bool currentDir     = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  m_sMimeType         = m_lstItems.first()->mimetype();
  mode_t mode         = m_lstItems.first()->mode();
  bool bTrashIncluded = false;
  bool bCanChangeSharing = false;
  m_lstPopupURLs.clear();
  int id = 0;
  if( m_sMimeType=="inode/directory" && m_lstItems.first()->isLocalFile())
    bCanChangeSharing=true;
  setFont(KGlobalSettings::menuFont());
  m_pluginList.setAutoDelete( true );
  m_ownActions.setHighlightingEnabled( true );

  attrName = QString::fromLatin1( "name" );

  prepareXMLGUIStuff();
  m_builder = new KonqPopupMenuGUIBuilder( this );
  m_factory = new KXMLGUIFactory( m_builder );

  KURL url;
  KFileItemListIterator it ( m_lstItems );
  // Check whether all URLs are correct
  for ( ; it.current(); ++it )
  {
    url = (*it)->url();

    // Build the list of URLs
    m_lstPopupURLs.append( url );

    // Determine if common mode among all URLs
    if ( mode != (*it)->mode() )
      mode = 0; // modes are different => reset to 0

    // Determine if common mimetype among all URLs
    if ( m_sMimeType != (*it)->mimetype() )
      m_sMimeType = QString::null; // mimetypes are different => null

    if ( !bTrashIncluded &&
         (*it)->url().isLocalFile() &&
         (*it)->url().path( 1 ) == KGlobalSettings::trashPath() )
        bTrashIncluded = true;

    if ( sReading )
      sReading = KProtocolInfo::supportsReading( url );

    if ( sWriting )
      sWriting = KProtocolInfo::supportsWriting( url );

    if ( sDeleting )
      sDeleting = KProtocolInfo::supportsDeleting( url );

    if ( sMoving )
      sMoving = KProtocolInfo::supportsMoving( url );
  }
  // Be on the safe side when including the trash
  if ( bTrashIncluded )
  {
      sMoving = false;
      sDeleting = false;
  }
  //check if current url is trash
  url = m_sViewURL;
  url.cleanPath();

  m_info.m_Reading = sReading;
  m_info.m_Writing = sWriting;
  m_info.m_Deleting = sDeleting;
  m_info.m_Moving = sMoving;
  m_info.m_TrashIncluded = bTrashIncluded;

  bool isCurrentTrash = ( url.isLocalFile() &&
                          url.path(1) == KGlobalSettings::trashPath() );

  //check if url is current directory
  if ( m_lstItems.count() == 1 )
  {
    KURL firstPopupURL ( m_lstItems.first()->url() );
    firstPopupURL.cleanPath();
    //kdDebug(1203) << "View path is " << url.url() << endl;
    //kdDebug(1203) << "First popup path is " << firstPopupURL.url() << endl;
    currentDir = firstPopupURL.cmp( url, true /* ignore_trailing */ );
  }

  clear();

  //////////////////////////////////////////////////////////////////////////

  KAction * act;

  addMerge( "konqueror" );

  bool isKDesktop = QCString(  kapp->name() ) == "kdesktop";
  QString openStr = isKDesktop ? i18n( "&Open" ) : i18n( "Open in New &Window" );
  KAction *actNewView = new KAction( openStr, "window_new", 0, this, SLOT( slotPopupNewView() ), &m_ownActions, "newview" );
  if ( !isKDesktop )
    actNewView->setStatusText( i18n( "Open the document in a new window" ) );

  if ( ( isCurrentTrash && currentDir ) ||
       ( m_lstItems.count() == 1 && bTrashIncluded ) )
  {
    addAction( actNewView );
    addGroup( "tabhandling" );
    addSeparator();

    act = new KAction( i18n( "&Empty Trash Bin" ), 0, this, SLOT( slotPopupEmptyTrashBin() ), &m_ownActions, "empytrash" );
    addAction( act );
  }
  else
  {
    if ( S_ISDIR(mode) && sWriting ) // A dir, and we can create things into it
    {
      if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
      {
        // As requested by KNewMenu :
        m_pMenuNew->slotCheckUpToDate();
        m_pMenuNew->setPopupFiles( m_lstPopupURLs );

        addAction( m_pMenuNew );

        addSeparator();
      }
      else
      {
        KAction *actNewDir = new KAction( i18n( "Create Director&y..." ), "folder_new", 0, this, SLOT( slotPopupNewDir() ), &m_ownActions, "newdir" );
        addAction( actNewDir );
        addSeparator();
      }
    }

    // hack for khtml pages/frames
    bool httpPage = (m_sViewURL.protocol().find("http", 0, false) == 0);

    if ( currentDir || httpPage ) // rmb on background or html frame
    {
      addAction( "up" );
      addAction( "back" );
      addAction( "forward" );
      if ( currentDir ) // khtml adds a different "reload frame" for frames
        addAction( "reload" );
      addGroup( "reload" );
      addSeparator();
    }

    // "open in new window" always available
    addAction( actNewView );
    addGroup( "tabhandling" );
    bool separatorAdded = false;

    if ( !currentDir && sReading ) {
        addSeparator();
        separatorAdded = true;
      if ( sDeleting ) {
        addAction( "undo" );
        addAction( "cut" );
      }
      addAction( "copy" );
    }

    if ( S_ISDIR(mode) && sWriting ) {
        if ( !separatorAdded )
            addSeparator();
        if ( currentDir )
            addAction( "paste" );
        else
            addAction( "pasteto" );
    }

    // The actions in this group are defined in PopupMenuGUIClient
    // When defined, it includes a separator before the 'find' action
    addGroup( "find" );

    if (!currentDir)
    {
        if ( sReading || sWriting ) // only if we added an action above
            addSeparator();

        if ( m_lstItems.count() == 1 && sWriting )
            addAction("rename");

        if ( sMoving )
            addAction( "trash" );

        if ( sDeleting ) {
            addAction( "del" );
            //if ( m_sViewURL.isLocalFile() )
            //    addAction( "shred" );
        }
    }
  }

  act = new KAction( i18n( "&Add to Bookmarks" ), "bookmark_add", 0, this, SLOT( slotPopupAddToBookmark() ), &m_ownActions, "addbookmark" );
  addAction( act );

  //////////////////////////////////////////////////////

  QValueList<KDEDesktopMimeType::Service> builtin;
  QValueList<KDEDesktopMimeType::Service> user;

  // 1 - Look for builtin and user-defined services
  if ( m_sMimeType == "application/x-desktop" && m_lstItems.count() == 1 && m_lstItems.first()->url().isLocalFile() ) // .desktop file
  {
      // get builtin services, like mount/unmount
      builtin = KDEDesktopMimeType::builtinServices( m_lstItems.first()->url() );
      user = KDEDesktopMimeType::userDefinedServices( m_lstItems.first()->url().path(), url.isLocalFile() );
  }

  // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)
  QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/servicemenus/" );
  QStringList::ConstIterator dIt = dirs.begin();
  QStringList::ConstIterator dEnd = dirs.end();

  for (; dIt != dEnd; ++dIt )
  {
      QDir dir( *dIt );

      QStringList entries = dir.entryList( QDir::Files );
      QStringList::ConstIterator eIt = entries.begin();
      QStringList::ConstIterator eEnd = entries.end();

      for (; eIt != eEnd; ++eIt )
      {
          KSimpleConfig cfg( *dIt + *eIt, true );

          cfg.setDesktopGroup();

          if ( cfg.hasKey( "X-KDE-AuthorizeAction") )
          {
              bool ok = true;
              QStringList list = cfg.readListEntry("X-KDE-AuthorizeAction");
              if (kapp && !list.isEmpty())
              {
                  for(QStringList::ConstIterator it = list.begin();
                      it != list.end();
                      ++it)
                  {
                      if (!kapp->authorize((*it).stripWhiteSpace()))
                      {
                          ok = false;
                          break;
                      }
                  }
              }
              if (!ok)
                continue;
          }

          if ( cfg.hasKey( "Actions" ) && cfg.hasKey( "ServiceTypes" ) )
          {
              QStringList types = cfg.readListEntry( "ServiceTypes" );
              bool ok = !m_sMimeType.isNull() && types.contains( m_sMimeType );
              if ( !ok ) {
                  ok = (types[0] == "all/all" ||
                        types[0] == "allfiles" /*compat with KDE up to 3.0.3*/);
                  if ( !ok && types[0] == "all/allfiles" )
                  {
                      ok = (m_sMimeType != "inode/directory"); // ## or inherits from it
                  }
              }
              if ( ok )
              {
                  user += KDEDesktopMimeType::userDefinedServices( *dIt + *eIt, url.isLocalFile() );
              }
          }
      }
  }

  KTrader::OfferList offers;
      // if check m_sMimeType.isNull (no commom mime type) set it to all/all
      // 3 - Query for applications
      offers = KTrader::self()->query( m_sMimeType.isNull( ) ? QString::fromLatin1( "all/all" ) : m_sMimeType ,
   "Type == 'Application' and DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'" );


  //// Ok, we have everything, now insert

  m_mapPopup.clear();
  m_mapPopupServices.clear();

  if ( !offers.isEmpty() )
  {
      // First block, app and preview offers
      addSeparator();

      id = 1;

      QDomElement menu = m_menuElement;

      if ( offers.count() > 1 ) // submenu 'open with'
      {
        menu = m_doc.createElement( "menu" );
	menu.setAttribute( "name", "openwith submenu" );
        m_menuElement.appendChild( menu );
        QDomElement text = m_doc.createElement( "text" );
        menu.appendChild( text );
        text.appendChild( m_doc.createTextNode( i18n("&Open With") ) );
      }

      if ( menu == m_menuElement ) // no submenu -> open with... above the single offer
      {
        KAction *openWithAct = new KAction( i18n( "&Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
        addAction( openWithAct, menu );
      }

      KTrader::OfferList::ConstIterator it = offers.begin();
      for( ; it != offers.end(); it++ )
      {
        QCString nam;
        nam.setNum( id );

        act = new KAction( (*it)->name(), (*it)->pixmap( KIcon::Small ), 0,
                           this, SLOT( slotRunService() ),
                           &m_ownActions, nam.prepend( "appservice_" ) );
        addAction( act, menu );

        m_mapPopup[ id++ ] = *it;
      }

      if ( menu != m_menuElement ) // submenu
      {
        addSeparator( menu );
        KAction *openWithAct = new KAction( i18n( "&Other..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
        addAction( openWithAct, menu ); // Other...
      }
  }
  else // no app offers -> Open With...
  {
      addSeparator();
      act = new KAction( i18n( "Open With..." ), 0, this, SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
      addAction( act );
  }

  addGroup( "preview" );

  addSeparator();

  // Second block, builtin + user
  if ( !user.isEmpty() || !builtin.isEmpty() )
  {
      bool insertedOffer = false;

      QValueList<KDEDesktopMimeType::Service>::Iterator it2 = user.begin();
      for( ; it2 != user.end(); ++it2 )
      {
        if ((*it2).m_display == true)
        {
          QCString nam;
          nam.setNum( id );
          act = new KAction( (*it2).m_strName, 0, this, SLOT( slotRunService() ), &m_ownActions, nam.prepend( "userservice_" ) );

          if ( !(*it2).m_strIcon.isEmpty() )
          {
            QPixmap pix = SmallIcon( (*it2).m_strIcon );
            act->setIconSet( pix );
          }

          addAction( act, m_menuElement ); // Add to toplevel menu

          m_mapPopupServices[ id++ ] = *it2;
          insertedOffer = true;
        }
      }

      it2 = builtin.begin();
      for( ; it2 != builtin.end(); ++it2 )
      {
        QCString nam;
        nam.setNum( id );

        act = new KAction( (*it2).m_strName, 0, this, SLOT( slotRunService() ), &m_ownActions, nam.prepend( "builtinservice_" ) );

        if ( !(*it2).m_strIcon.isEmpty() )
        {
          QPixmap pix = SmallIcon( (*it2).m_strIcon );
          act->setIconSet( pix );
        }

        addAction( act, m_menuElement );

        m_mapPopupServices[ id++ ] = *it2;
        insertedOffer = true;
      }

      if ( insertedOffer )
        addSeparator();
  }
  addPlugins( ); // now it's time to add plugins

  if ( !m_sMimeType.isEmpty() && showPropertiesAndFileType )
  {
      act = new KAction( i18n( "&Edit File Type..." ), 0, this, SLOT( slotPopupMimeType() ),
                       &m_ownActions, "editfiletype" );
      addAction( act );
  }

  if ( KPropertiesDialog::canDisplay( m_lstItems ) && showPropertiesAndFileType )
  {
      act = new KAction( i18n( "&Properties" ), 0, this, SLOT( slotPopupProperties() ),
                         &m_ownActions, "properties" );
      addAction( act );
  }

  while ( !m_menuElement.lastChild().isNull() &&
            m_menuElement.lastChild().toElement().tagName().lower() == "separator" )
    m_menuElement.removeChild( m_menuElement.lastChild() );

  if( bCanChangeSharing)
  {
   if(KFileShare::authorization()==KFileShare::Authorized)
   {
       addSeparator();
       QString label;
       label=i18n("Share");

       act = new KAction( label, 0, this, SLOT( slotOpenShareFileDialog() ),
                  &m_ownActions, "sharefile" );
       addAction( act );
   }
  }


  addMerge( 0 );

  m_factory->addClient( this );
}

void KonqPopupMenu::slotOpenShareFileDialog()
{
    //kdDebug()<<"KonqPopupMenu::slotOpenShareFileDialog()\n";
    // It may be that the kfileitem was created by hand
    // (see KonqKfmIconView::slotMouseButtonPressed)
    // In that case, we can get more precise info in the properties
    // (like permissions) if we stat the URL.
    if ( m_lstItems.count() == 1 )
    {
        KFileItem * item = m_lstItems.first();
        if (item->entry().count() == 0) // this item wasn't listed by a slave
        {
            // KPropertiesDialog will use stat to get more info on the file
            KPropertiesDialog*dlg= new KPropertiesDialog( item->url(), d->m_parentWidget );
            dlg->showFileSharingPage();

            return;
        }
    }
    KPropertiesDialog*dlg=new KPropertiesDialog( m_lstItems, d->m_parentWidget );
    dlg->showFileSharingPage();
}

KonqPopupMenu::~KonqPopupMenu()
{
  m_pluginList.clear();
  delete m_factory;
  delete m_builder;
  delete d;
  kdDebug(1203) << "~KonqPopupMenu leave" << endl;
}

void KonqPopupMenu::setURLTitle( const QString& urlTitle )
{
    d->m_urlTitle = urlTitle;
}

void KonqPopupMenu::slotPopupNewView()
{
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it);
}

void KonqPopupMenu::slotPopupNewDir()
{
    KLineEditDlg l( i18n("Enter directory name:"), i18n("Directory"), d->m_parentWidget );
    l.setCaption(i18n("New Directory"));
    if ( l.exec() )
    {
        QString name = KIO::encodeFileName( l.text() );
        KURL::List::ConstIterator it = m_lstPopupURLs.begin();
        for ( ; it != m_lstPopupURLs.end(); it++ )
        {
            KURL url(*it);
            url.addPath( name );
            kdDebug() << "KonqPopupMenu::slotPopupNewDir creating dir " << url.url() << endl;
            KonqOperations::mkdir( 0L, url );
        }
    }
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash();
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KRun::displayOpenWithDialog( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmarkGroup root = m_pManager->root();
  if ( m_lstPopupURLs.count() == 1 ) {
    KURL url = m_lstPopupURLs.first();
    QString title = d->m_urlTitle.isEmpty() ? url.prettyURL() : d->m_urlTitle;
    root.addBookmark( m_pManager, title, url.url() );
  }
  else
  {
    KURL::List::ConstIterator it = m_lstPopupURLs.begin();
    for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( m_pManager, (*it).prettyURL(), (*it).url() );
  }
  m_pManager->emitChanged( root );
}

void KonqPopupMenu::slotRunService()
{
  QCString senderName = sender()->name();
  int id = senderName.mid( senderName.find( '_' ) + 1 ).toInt();

  // Is it a usual service (application)
  QMap<int,KService::Ptr>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( **it, m_lstPopupURLs );
    return;
  }

  // Is it a service specific to desktop entry files ?
  QMap<int,KDEDesktopMimeType::Service>::Iterator it2 = m_mapPopupServices.find( id );
  if ( it2 != m_mapPopupServices.end() )
  {
      KDEDesktopMimeType::executeService( m_lstPopupURLs, it2.data() );
  }

  return;
}

void KonqPopupMenu::slotPopupMimeType()
{
  KonqOperations::editMimeType( m_sMimeType );
}

void KonqPopupMenu::slotPopupProperties()
{
    // It may be that the kfileitem was created by hand
    // (see KonqKfmIconView::slotMouseButtonPressed)
    // In that case, we can get more precise info in the properties
    // (like permissions) if we stat the URL.
    if ( m_lstItems.count() == 1 )
    {
        KFileItem * item = m_lstItems.first();
        if (item->entry().count() == 0) // this item wasn't listed by a slave
        {
            // KPropertiesDialog will use stat to get more info on the file
            (void) new KPropertiesDialog( item->url(), d->m_parentWidget );
            return;
        }
    }
    (void) new KPropertiesDialog( m_lstItems, d->m_parentWidget );
}

KAction *KonqPopupMenu::action( const QDomElement &element ) const
{
  QCString name = element.attribute( attrName ).ascii();

  KAction *res = m_ownActions.action( name );

  if ( !res )
    res = m_actions.action( name );

  if ( !res && strcmp( name, m_pMenuNew->name() ) == 0 )
    return m_pMenuNew;

  return res;
}
KActionCollection *KonqPopupMenu::actionCollection() const
{
  return const_cast<KActionCollection *>( &m_ownActions );
}

QString KonqPopupMenu::mimeType( ) const {
    return m_sMimeType;
}
KonqPopupMenu::ProtocolInfo KonqPopupMenu::protocolInfo() const
{
  return m_info;
}
void KonqPopupMenu::addPlugins( ){
	// search for Konq_PopupMenuPlugins inspired by simons kpropsdlg
	//search for a plugin with the right protocol
	KTrader::OfferList plugin_offers;
        unsigned int pluginCount = 0;
	plugin_offers = KTrader::self()->query( m_sMimeType.isNull() ? QString::fromLatin1( "all/all" ) : m_sMimeType , "'KonqPopupMenu/Plugin' in ServiceTypes");
        if ( plugin_offers.isEmpty() )
	  return; // no plugins installed do not bother about it

	KTrader::OfferList::ConstIterator iterator = plugin_offers.begin( );
	KTrader::OfferList::ConstIterator end = plugin_offers.end( );

	addGroup( "plugins" );
	// travers the offerlist
	for(; iterator != end; ++iterator, ++pluginCount ){
		KonqPopupMenuPlugin *plugin =
			KParts::ComponentFactory::
			createInstanceFromLibrary<KonqPopupMenuPlugin>( (*iterator)->library().local8Bit(),
									this,
									(*iterator)->name().latin1() );
		if ( !plugin )
			continue;
                QString pluginClientName = QString::fromLatin1( "Plugin%1" ).arg( pluginCount );
                addMerge( pluginClientName );
                plugin->domDocument().documentElement().setAttribute( "name", pluginClientName );
		m_pluginList.append( plugin );
		insertChildClient( plugin );
	}

	addMerge( "plugins" );
	addSeparator();
}
KURL KonqPopupMenu::url( ) const {
  return m_sViewURL;
}
KFileItemList KonqPopupMenu::fileItemList( ) const {
  return m_lstItems;
}
KURL::List KonqPopupMenu::popupURLList( ) const {
  return m_lstPopupURLs;
}
/**
	Plugin
*/

KonqPopupMenuPlugin::KonqPopupMenuPlugin( KonqPopupMenu *parent, const char *name )
    : QObject( parent, name ) {
}
KonqPopupMenuPlugin::~KonqPopupMenuPlugin( ){

}
#include "konq_popupmenu.moc"
