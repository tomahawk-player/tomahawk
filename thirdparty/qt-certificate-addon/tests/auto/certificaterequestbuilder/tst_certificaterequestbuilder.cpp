#include <QtTest/QtTest>

#include "certificaterequest.h"
#include "certificaterequestbuilder.h"

QT_USE_NAMESPACE_CERTIFICATE

class tst_CertificateRequestBuilder : public QObject
{
    Q_OBJECT

private slots:
    void version();
    void entries();
};

void tst_CertificateRequestBuilder::version()
{
    CertificateRequestBuilder builder;
    builder.setVersion(1);
    QCOMPARE(builder.version(), 1);
}

void tst_CertificateRequestBuilder::entries()
{
    CertificateRequestBuilder builder;
    builder.setVersion(1);
    builder.addNameEntry(Certificate::EntryCountryName, "GB");
    builder.addNameEntry(Certificate::EntryOrganizationName, "Westpoint");
    builder.addNameEntry(Certificate::EntryOrganizationName, "West");
    builder.addNameEntry(Certificate::EntryOrganizationalUnitName, "My Unit");
    builder.addNameEntry(Certificate::EntryLocalityName, "My Locality");
    builder.addNameEntry(Certificate::EntryStateOrProvinceName, "My State");
    builder.addNameEntry(Certificate::EntryEmail, "test@example.com");
    builder.addNameEntry(Certificate::EntryCommonName, "www.example.com");

    QFile f("keys/leaf.key");
    f.open(QIODevice::ReadOnly);
    QSslKey key(&f, QSsl::Rsa);
    f.close();

    builder.setKey(key);
    CertificateRequest req = builder.signedRequest(key);

    QStringList countryName;
    countryName << "GB";
    QCOMPARE(countryName, req.nameEntryInfo(Certificate::EntryCountryName));

    QStringList organizationName;
    organizationName << "Westpoint";
    organizationName << "West";
    QCOMPARE(organizationName, req.nameEntryInfo(Certificate::EntryOrganizationName));

    QStringList organizationalUnitName;
    organizationalUnitName << "My Unit";
    QCOMPARE(organizationalUnitName, req.nameEntryInfo(Certificate::EntryOrganizationalUnitName));

    QStringList stateOrProvinceName;
    stateOrProvinceName << "My State";
    QCOMPARE(stateOrProvinceName, req.nameEntryInfo(Certificate::EntryStateOrProvinceName));

    QStringList email;
    email << "test@example.com";
    QCOMPARE(email, req.nameEntryInfo(Certificate::EntryEmail));

    QStringList commonName;
    commonName << "www.example.com";
    QCOMPARE(commonName, req.nameEntryInfo(Certificate::EntryCommonName));
}


QTEST_MAIN(tst_CertificateRequestBuilder)
#include "tst_certificaterequestbuilder.moc"
