#pragma once

#include "dom/Dom.h"

#include <QString>
#include <QVector>

namespace dom {

/// CSS selector engine for the selectors the manga/novel extension scripts use, with browser semantics.
///
/// Supported:
///   Type, `*`, #id, .class (with CSS escapes, e.g. .bg-\[\#1C1924\])
///   [attr], [attr=v], [attr*=v], [attr^=v], [attr$=v], [attr~=v], [attr|=v]  (quoted or bare values, `i` flag)
///   Combinators: descendant (space), child (>), adjacent sibling (+), general sibling (~), comma lists
///   Leading `>` ("direct children of the scope element"), which browsers reject but the scripts rely on
///   Pseudo-classes: :first-child :last-child :only-child :nth-child() :nth-last-child()
///                   :first-of-type :last-of-type :only-of-type :nth-of-type() :nth-last-of-type()
///                   :empty :root :not() :is() :where() :has() and the jQuery extension :contains("text")
///
/// The grammar is strict. A selector that uses anything else (an unknown pseudo-class, a stray character, an
/// unterminated bracket) is INVALID: it is logged once and matches nothing. It is never silently reduced to a
/// simpler selector, because that would return the wrong elements instead of none.
///
/// Matching follows querySelectorAll: ancestors above the scope element take part in descendant/child matching.
/// Only a leading `>` and the arguments of :has() are relative to a scope element.

/// First element in the subtree of `scope` (scope itself is not tested) that matches `selector`, or kNone.
NodeId querySelector(const Document &doc, NodeId scope, const QString &selector);

/// All matching elements in the subtree of `scope`, in document order.
QVector<NodeId> querySelectorAll(const Document &doc, NodeId scope, const QString &selector);

/// Nearest ancestor of `startNode`, including `startNode` itself, that matches `selector`, or kNone.
NodeId closest(const Document &doc, NodeId startNode, const QString &selector);

/// True when `selector` parses. On failure *error (if given) says what is wrong and where.
bool isValidSelector(const QString &selector, QString *error = nullptr);

} // namespace dom
