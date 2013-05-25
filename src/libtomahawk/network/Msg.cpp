#include "Msg.h"

#include <qjson/parser.h>

QVariant& Msg::json() {
    Q_ASSERT( is(JSON) );
    Q_ASSERT( !is(COMPRESSED) );

    if( !m_json_parsed )
    {
        QJson::Parser p;
        bool ok;
        m_json = p.parse( m_payload, &ok );
        m_json_parsed = true;
    }
    return m_json;
}