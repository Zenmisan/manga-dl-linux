/* extension_bridge.h - Vala C bridge for QuickJS */
#ifndef EXTENSION_BRIDGE_H
#define EXTENSION_BRIDGE_H

#include <glib.h>

typedef struct ExtensionBridge ExtensionBridge;

ExtensionBridge* extension_bridge_new(void);
void             extension_bridge_free    (ExtensionBridge *bridge);
gboolean         extension_bridge_load    (ExtensionBridge *bridge, const char *js_source, GError **error);
char*            extension_bridge_search  (ExtensionBridge *bridge, const char *query, int page, GError **error);
char*            extension_bridge_detail  (ExtensionBridge *bridge, const char *manga_id, GError **error);
char*            extension_bridge_pages   (ExtensionBridge *bridge, const char *chapter_id, GError **error);

#endif