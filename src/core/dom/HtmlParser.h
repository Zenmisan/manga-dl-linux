#pragma once

#include "dom/Dom.h"

#include <QString>

#include <memory>

namespace dom {

/// Parses HTML the way a browser would, as far as scraping needs: tolerant of broken markup, always yields
/// `html > head + body`, inserts the implied `tbody`, closes `p`/`li`/`td`/... automatically, decodes entities,
/// keeps `script`/`style`/`textarea`/`title` content as raw text.
///
/// Deliberately not implemented: the adoption-agency algorithm for misnested formatting tags, foster parenting of
/// content inside tables, and the full 2231-name entity table (a table of ~300 common names is built in; unknown
/// names are left as literal text).
std::unique_ptr<Document> parseHtml(const QString &html);

/// Decodes character references (`&amp;`, `&#169;`, `&#x1F600;`, ...) in a text snippet.
QString decodeHtmlEntities(const QString &text);

} // namespace dom
