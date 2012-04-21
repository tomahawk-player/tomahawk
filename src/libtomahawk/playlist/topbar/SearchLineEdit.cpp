/**
 * Copyright (c) 2009, Benjamin C. Meyer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "SearchLineEdit.h"

#include "ClearButton.h"
#include "SearchButton.h"

SearchLineEdit::SearchLineEdit(QWidget *parent)
    : LineEdit(parent)
{
    init();
}

void SearchLineEdit::init()
{
    // search button on the left
    m_searchButton = new SearchButton(this);
    addWidget(m_searchButton, LeftSide);

    // clear button on the right
    m_clearButton = new ClearButton(this);
    connect(m_clearButton, SIGNAL(clicked()),
            this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)),
            m_clearButton, SLOT(textChanged(const QString&)));
    addWidget(m_clearButton, RightSide);
    m_clearButton->hide();

    setWidgetSpacing(5);

    updateTextMargins();
    setInactiveText(tr("Search"));
}

ClearButton *SearchLineEdit::clearButton() const
{
    return m_clearButton;
}

SearchButton *SearchLineEdit::searchButton() const
{
    return m_searchButton;
}
