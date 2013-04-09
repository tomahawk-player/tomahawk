
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

/*!
        \class QxtHtmlTemplate
        \inmodule QxtWeb
        \brief The QxtHtmlTemplate class provides a basic HTML template engine

        open a file containing html code and php style variables.
        use the square bracket operators to assign content for a variable

        \code
        QxtHtmlTemplate index;
        if(!index.open)
                return 404;
        index["content"]="hello world";
        echo()<<index.render();
        \endcode
        the realatet html code would look like:
        \code
        <html>
        <head>
                <title>Test Page</title>
        </head>
                <?=content?>
        </html>
        \endcode

        funny storry: whe are using this class to make our documentation (eat your own dogfood, you know ;).
        but when we where parsing exactly this file you read right now the first time, QxtHtmlTemplate got stuck in an infinite loop. guess why. becouse of that example above :D
        So be warned: when you assign content to a variable that contains the variable name itself, render() will never return.


*/

/*!
        \fn QxtHtmlTemplate::open(const QString& filename)
        Opens \a filename. Returns \c true on success and \c false on failure.
        Note that it will also return false for an empty html file.
 */

/*!
        \fn QString QxtHtmlTemplate::render() const
        Uses the variables you set and renders the opened file.
        returns an empty string on failure.
        Does NOT take care of not assigned variables, they will remain in the returned string
 */

#include "qxthtmltemplate.h"
#include <QFile>
#include <QStringList>

/*!
    Constructs a new QxtHtmlTemplate.
 */
QxtHtmlTemplate::QxtHtmlTemplate() : QMap<QString, QString>()
{}

/*!
    Loads data \a d.
 */
void QxtHtmlTemplate::load(const QString& d)
{
    data = d;
}

bool QxtHtmlTemplate::open(const QString& filename)
{
    QFile f(filename);
    f.open(QIODevice::ReadOnly);
    data = QString::fromLocal8Bit(f.readAll());
    f.close();
    if (data.isEmpty())
    {
        qWarning("QxtHtmlTemplate::open(\"%s\") empty or nonexistent", qPrintable(filename));
        return false;
    }
    return true;
}

QString QxtHtmlTemplate::render() const
{
    ///try to preserve indention by parsing char by char and saving the last non-space character


    QString output = data;
    int lastnewline = 0;


    for (int i = 0;i < output.count();i++)
    {
        if (output.at(i) == '\n')
        {
            lastnewline = i;
        }

        if (output.at(i) == '<' && output.at(i + 1) == '?'  && output.at(i + 2) == '=')
        {
            int j = i + 3;
            QString var;

            for (int jj = j;jj < output.count();jj++)
            {
                if (output.at(jj) == '?' && output.at(jj + 1) == '>')
                {
                    j = jj;
                    break;
                }
                var += output.at(jj);
            }


            if (j == i)
            {
                qWarning("QxtHtmlTemplate::render()  unterminated <?= ");
                continue;
            }


            if (!contains(var))
            {
                qWarning("QxtHtmlTemplate::render()  unused variable \"%s\"", qPrintable(var));
                continue;
            }
            output.replace(i, j - i + 2, QString(value(var)).replace('\n', '\n' + QString(i - lastnewline - 1, QChar(' '))));

        }


    }

    return output;
}

