/* Xviewer - XVIEWER Plugin Engine
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-plugins-engine.h) by:
 * 	- Paolo Maggi <paolo@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __XVIEWER_PLUGIN_ENGINE_H__
#define __XVIEWER_PLUGIN_ENGINE_H__

#include <libpeas/peas-engine.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _XviewerPluginEngine XviewerPluginEngine;
typedef struct _XviewerPluginEngineClass XviewerPluginEngineClass;
typedef struct _XviewerPluginEnginePrivate XviewerPluginEnginePrivate;

#define XVIEWER_TYPE_PLUGIN_ENGINE            xviewer_plugin_engine_get_type()
#define XVIEWER_PLUGIN_ENGINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_PLUGIN_ENGINE, XviewerPluginEngine))
#define XVIEWER_PLUGIN_ENGINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XVIEWER_TYPE_PLUGIN_ENGINE, XviewerPluginEngineClass))
#define XVIEWER_IS_PLUGIN_ENGINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_PLUGIN_ENGINE))
#define XVIEWER_IS_PLUGIN_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XVIEWER_TYPE_PLUGIN_ENGINE))
#define XVIEWER_PLUGIN_ENGINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XVIEWER_TYPE_PLUGIN_ENGINE, XviewerPluginEngineClass))

struct _XviewerPluginEngine {
  PeasEngine parent;
  XviewerPluginEnginePrivate *priv;
};

struct _XviewerPluginEngineClass {
  PeasEngineClass parent_class;
};

G_GNUC_INTERNAL
GType xviewer_plugin_engine_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
XviewerPluginEngine* xviewer_plugin_engine_new (void);

G_END_DECLS

#endif  /* __XVIEWER_PLUGIN_ENGINE_H__ */
