#include <QSslKey>
#include <QtTest/QtTest>

#include "keybuilder.h"

QT_USE_NAMESPACE_CERTIFICATE

class tst_KeyBuilder : public QObject
{
    Q_OBJECT

private slots:
    void checkKeyLengths();
    void checkKeyChanges();
};

void tst_KeyBuilder::checkKeyLengths()
{
   QSslKey keyl = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthLow );
   QVERIFY(keyl.length() >= 1248);
   
   QSslKey keyn = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthNormal );
   QVERIFY(keyn.length() >= 2322);
   QVERIFY(keyn.length() > keyl.length());

#ifdef ENABLE_SLOW_TESTS
   QSslKey keyh = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthHigh );
   QVERIFY(keyh.length() >= 3248);
   QVERIFY(keyh.length() > keyn.length());

   QSslKey keyu = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthUltra );
   QVERIFY(keyu.length() >= 15424);
   QVERIFY(keyu.length() > keyh.length());
#endif // ENABLE_SLOW_TESTS
}

void tst_KeyBuilder::checkKeyChanges()
{
    QSslKey key1 = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthLow );
    QSslKey key2 = KeyBuilder::generate( QSsl::Rsa, KeyBuilder::StrengthLow );
    
    QVERIFY(key1.toPem() != key2.toPem());
}

QTEST_MAIN(tst_KeyBuilder)
#include "tst_keybuilder.moc"
