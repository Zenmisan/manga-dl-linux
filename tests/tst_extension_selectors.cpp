// tst_extension_selectors.cpp - every selector literal in the real extension scripts must be understood
//
// The selector engine is strict: a selector it cannot parse matches nothing. This test extracts every string
// literal passed to querySelector / querySelectorAll / closest / matches in assets/extensions/*.js and checks that the
// engine accepts all of them, so a script can never silently depend on something that is not supported.
#include <QtTest>

#include "dom/Selector.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>

namespace {

// JavaScript string-literal escapes, enough for selectors: \\ \' \" \n \t and "backslash + any char".
QString unescapeJs(const QString &s)
{
    QString out;
    for (int i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            const QChar next = s[++i];
            if (next == 'n')
                out += QLatin1Char('\n');
            else if (next == 't')
                out += QLatin1Char('\t');
            else
                out += next;
        } else {
            out += s[i];
        }
    }
    return out;
}

} // namespace

class TstExtensionSelectors : public QObject
{
    Q_OBJECT

private slots:
    void everySelectorInTheScriptsIsSupported()
    {
        const QDir dir(QStringLiteral(MANGADL_ASSETS_DIR "/extensions"));
        const QStringList files = dir.entryList({QStringLiteral("*.js")}, QDir::Files, QDir::Name);
        QVERIFY2(!files.isEmpty(), "no scripts found: run tools/sync-extensions.sh first");

        // A selector literal directly followed by ')' or ',' (so `'a' + x` concatenations are skipped).
        static const QRegularExpression literal(
            QStringLiteral("(?:querySelectorAll|querySelector|closest|matches)\\(\\s*"
                           "(?:'((?:[^'\\\\\\n]|\\\\.)*)'|\"((?:[^\"\\\\\\n]|\\\\.)*)\"|`((?:[^`\\\\]|\\\\.)*)`)"
                           "\\s*[),]"));

        QSet<QString> seen;
        QStringList invalid;
        int checked = 0;
        for (const QString &file : files) {
            QFile f(dir.filePath(file));
            QVERIFY(f.open(QIODevice::ReadOnly));
            const QString source = QString::fromUtf8(f.readAll());

            QRegularExpressionMatchIterator it = literal.globalMatch(source);
            while (it.hasNext()) {
                const QRegularExpressionMatch m = it.next();
                const QString raw = !m.captured(1).isNull() ? m.captured(1)
                    : !m.captured(2).isNull()               ? m.captured(2)
                                                            : m.captured(3);
                if (raw.contains(QStringLiteral("${")))
                    continue;  // a template literal with an expression: not a fixed selector
                const QString selector = unescapeJs(raw);
                if (seen.contains(selector))
                    continue;
                seen.insert(selector);
                ++checked;

                QString error;
                if (!dom::isValidSelector(selector, &error))
                    invalid.append(file + QStringLiteral(": ") + selector + QStringLiteral("   -> ") + error);
            }
        }

        qInfo() << "distinct selectors checked:" << checked;
        QVERIFY2(checked > 200, "far fewer selectors than expected: the extraction pattern is probably broken");
        QVERIFY2(invalid.isEmpty(),
                 qPrintable(QStringLiteral("%1 selector(s) the engine cannot parse:\n").arg(invalid.size())
                            + invalid.join(QLatin1Char('\n'))));
    }
};

QTEST_MAIN(TstExtensionSelectors)
#include "tst_extension_selectors.moc"
