/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

#ifndef __konq_settings_h__
#define __konq_settings_h__

class KConfig;
#include <qcolor.h>
#include <qstring.h>

/**
 * The class KonqFMSettings holds the general settings for the
 * icon/tree views in konqueror/kdesktop.
 * There is no 'local' (per-URL) instance of it.
 * All those settings can only be changed in kcmkonq.
 *
 * Using this class from konqueror and from kdesktop return
 * different settings, since the config file is different.
 * konquerorrc, group "FMSettings", and kdesktoprc, group "FMSettings"
 * The kcontrol modules handles both files, depending where
 * it's called from.
 */

class KonqFMSettings
{
protected:
  /** @internal
   * Constructs a KonqFMSettings instance from a config file.
   */
  KonqFMSettings( KConfig * config );

  /** Called by constructor and reparseConfiguration */
  void init( KConfig * config );

  /** Destructor. Don't delete any instance by yourself. */
  virtual ~KonqFMSettings() {};

public:

  /**
   * The static instance of KonqFMSettings
   */
  static KonqFMSettings * settings();

  /**
   * Reparse the configuration to update the already-created instances
   * Warning : you need to call kapp->config()->reparseConfiguration() first
   * (This is not done here so that the caller can avoid too much reparsing
   *  if having several classes from the same config file)
   */
  static void reparseConfiguration();

  // Use settings (and mimetype definition files)
  // to find whether to embed a certain service type or not
  // Only makes sense in konqueror.
  bool shouldEmbed( const QString & serviceType );

  // Behaviour settings
  bool wordWrapText() { return m_bWordWrapText; }
  bool underlineLink() { return m_underlineLink; }
  bool alwaysNewWin() { return m_alwaysNewWin; }
  QString homeURL() { return m_homeURL; }
  bool treeFollow() { return m_bTreeFollow; }

  // Font settings
  const QString& stdFontName() { return m_strStdFontName; }
  int fontSize() { return m_iFontSize; }

  // Color settings
  const QColor& bgColor() { return m_bgColor; }
  const QColor& normalTextColor() { return m_normalTextColor; }
  const QColor& highlightedTextColor() { return m_highlightedTextColor; }

protected:
  static KonqFMSettings * s_pSettings;

  bool m_underlineLink;
  bool m_alwaysNewWin;
  bool m_bTreeFollow;

  bool m_embedText;
  bool m_embedImage;
  bool m_embedOther;

  QString m_strStdFontName;
  QString m_strFixedFontName;
  int m_iFontSize;

  QColor m_bgColor;
  QColor m_normalTextColor;
  QColor m_highlightedTextColor;

  bool m_bWordWrapText;

  QString m_homeURL;

private:
  // There is no default constructor. Use the provided ones.
  KonqFMSettings();
  // No copy constructor either. What for ?
  KonqFMSettings( const KonqFMSettings &);
};

#endif
