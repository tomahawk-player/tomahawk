#include <QtTest/QtTest>

#include "certificaterequest.h"

QT_USE_NAMESPACE_CERTIFICATE

class tst_CertificateRequest : public QObject
{
    Q_OBJECT

private slots:
    void checkNull();
    void loadCrq();
    void checkEntryAttributes();
    void checkEntries();
    void checkToText();
};

void tst_CertificateRequest::checkNull()
{
    CertificateRequest csr;
    QVERIFY(csr.isNull());
}

void tst_CertificateRequest::loadCrq()
{
    QFile f("requests/test-ocsp-good-req.pem");
    f.open(QIODevice::ReadOnly);
    CertificateRequest csr(&f);
    f.close();

    QVERIFY(!csr.isNull());
    QVERIFY(csr.version() == 1);

    QFile f2("requests/test-ocsp-good-req.pem");
    f2.open(QIODevice::ReadOnly);
    QByteArray filePem = f2.readAll();
    f2.close();

    QVERIFY(filePem == csr.toPem());
}

void tst_CertificateRequest::checkEntryAttributes()
{
    QFile f("requests/test-ocsp-good-req.pem");
    f.open(QIODevice::ReadOnly);
    CertificateRequest csr(&f);
    f.close();

    QList<QByteArray> attrs;
    attrs << "2.5.4.3" << "2.5.4.8" << "2.5.4.6" << "1.2.840.113549.1.9.1" << "2.5.4.10";

    QCOMPARE(attrs, csr.nameEntryAttributes());
}

void tst_CertificateRequest::checkEntries()
{
    QFile f("requests/test-ocsp-good-req.pem");
    f.open(QIODevice::ReadOnly);
    CertificateRequest csr(&f);
    f.close();

    QStringList commonName;
    commonName << "example.com";
    QVERIFY(commonName ==  csr.nameEntryInfo(Certificate::EntryCommonName));

    QStringList organizationName;
    organizationName << "Some organisation";
    QVERIFY(organizationName ==  csr.nameEntryInfo(Certificate::EntryOrganizationName));

    QStringList countryName;
    countryName << "UK";
    QVERIFY(countryName ==  csr.nameEntryInfo(Certificate::EntryCountryName));

    QStringList email;
    email << "test@example.com";
    QVERIFY(email ==  csr.nameEntryInfo(Certificate::EntryEmail));

    QStringList stateOrProvinceName;
    stateOrProvinceName << "Lancashire";
    QVERIFY(stateOrProvinceName ==  csr.nameEntryInfo(Certificate::EntryStateOrProvinceName));

    QStringList localityName;
    QVERIFY(localityName == csr.nameEntryInfo(Certificate::EntryLocalityName));
}

void tst_CertificateRequest::checkToText()
{
    QFile f("requests/test-ocsp-good-req.pem");
    f.open(QIODevice::ReadOnly);
    CertificateRequest csr(&f);
    f.close();

    QLatin1String text("PKCS #10 Certificate Request Information:\n" \
                       "\tVersion: 1\n" \
                       "\tSubject: CN=example.com,ST=Lancashire,C=UK,EMAIL=test@example.com,O=Some organisation\n" \
                       "\tSubject Public Key Algorithm: RSA\n" \
                       "\t\tModulus (bits 1024):\n" \
                       "\t\t\t97:c9:92:27:81:a7:4c:64:82:a2:30:d6:07:b7:57:e0\n" \
                       "\t\t\t9c:ea:cd:eb:53:be:ea:b6:b5:47:66:d0:68:54:25:a7\n" \
                       "\t\t\ted:21:5c:dc:fd:da:41:f6:c7:c0:35:ae:97:72:fd:8b\n" \
                       "\t\t\taf:29:3d:38:5a:67:8b:39:8a:ce:86:25:0f:38:a7:b5\n" \
                       "\t\t\t38:b3:8e:81:f0:ea:79:99:cb:f5:23:64:55:f3:4b:a4\n" \
                       "\t\t\tb6:23:64:29:ea:ba:f3:29:52:a7:7f:32:dc:0d:b6:d9\n" \
                       "\t\t\td4:e6:13:de:01:41:86:9a:2d:8f:bb:0c:18:88:09:ac\n" \
                       "\t\t\td4:6a:e9:cb:8a:17:8a:85:09:a6:ae:a6:1c:05:e9:55\n" \
                       "\t\tExponent:\n" \
                       "\t\t\t01:00:01\n" \
                       "Other Information:\n" \
                       "\tPublic Key Id:\n" \
                       "\t\tad19b31110ef2ef08ed382e1f4a1766bcbf9d562\n");

    QCOMPARE(text, csr.toText());
}

QTEST_MAIN(tst_CertificateRequest)
#include "tst_certificaterequest.moc"
