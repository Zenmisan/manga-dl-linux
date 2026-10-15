#include "extensions/ExtensionModels.h"

QString SourceError::toString() const
{
    return message.isEmpty() ? QStringLiteral("Unknown error") : message;
}

// static
//
// JsWorker tags the errors it produces itself ("[network] ...", "[timeout] ...", "[load] ...", "[call] ...").
// Everything else is the text of an exception thrown by the extension script, which is reported as a ParseError
// unless the caller says otherwise.
SourceError SourceError::fromText(const QString &text, SourceErrorKind fallback)
{
    struct Tag {
        const char *prefix;
        SourceErrorKind kind;
    };
    static const Tag tags[] = {
        {"[network]", SourceErrorKind::NetworkError},
        {"[timeout]", SourceErrorKind::Timeout},
        {"[load]", SourceErrorKind::ScriptLoadError},
        {"[call]", SourceErrorKind::ScriptLoadError},
    };
    for (const Tag &tag : tags) {
        const QString prefix = QString::fromLatin1(tag.prefix);
        if (text.startsWith(prefix))
            return SourceError{tag.kind, text.mid(prefix.size()).trimmed()};
    }
    return SourceError{fallback, text};
}
