#include <QByteArray>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QSslKey>
#include <QSslCertificate>

#include "keybuilder.h"
#include "certificaterequestbuilder.h"
#include "certificaterequest.h"
#include "certificatebuilder.h"
#include "randomgenerator.h"
#include "certificate.h"

QT_USE_NAMESPACE_CERTIFICATE

void save_key(const QString &filename, const QSslKey &key)
{
    QFile k(filename);
    k.open(QIODevice::WriteOnly);
    k.write(key.toPem());
    k.close();
}

void save_request(const QString &filename, CertificateRequest &req)
{
    QFile k(filename);
    k.open(QIODevice::WriteOnly);
    k.write(req.toPem());
    k.close();
}

void save_certificate(const QString &filename, const QSslCertificate &crt)
{
    QFile k(filename);
    k.open(QIODevice::WriteOnly);
    k.write(crt.toPem());
    k.close();
}

int main(int argc, char **argv)
{
    //
    // Create the CA key
    //
    QSslKey cakey = KeyBuilder::generate(QSsl::Rsa, KeyBuilder::StrengthNormal);
    save_key("ca.key", cakey);

    CertificateRequestBuilder careqbuilder;
    careqbuilder.setVersion(1);
    careqbuilder.setKey(cakey);
    careqbuilder.addNameEntry(Certificate::EntryCountryName, "GB");
    careqbuilder.addNameEntry(Certificate::EntryOrganizationName, "Westpoint CA Key");
    careqbuilder.addNameEntry(Certificate::EntryOrganizationName, "Westpoint");

    // Sign the request
    CertificateRequest careq = careqbuilder.signedRequest(cakey);
    save_request("ca.req", careq);

    //
    // Now make a certificate
    //
    CertificateBuilder cabuilder;
    cabuilder.setRequest(careq);

    cabuilder.setVersion(3);
    cabuilder.setSerial(RandomGenerator::getPositiveBytes(16));
    cabuilder.setActivationTime(QDateTime::currentDateTimeUtc());
    cabuilder.setExpirationTime(QDateTime::currentDateTimeUtc().addYears(10));
    cabuilder.setBasicConstraints(true);
    cabuilder.setKeyUsage(CertificateBuilder::UsageCrlSign|CertificateBuilder::UsageKeyCertSign);
    cabuilder.addSubjectKeyIdentifier();

    QSslCertificate cacert = cabuilder.signedCertificate(cakey);
    save_certificate("ca.crt", cacert);

    //
    // Now make an intermediate
    //
    QSslKey interkey = KeyBuilder::generate(QSsl::Rsa, KeyBuilder::StrengthNormal);
    save_key("inter.key", interkey);

    CertificateRequestBuilder interreqbuilder;
    interreqbuilder.setVersion(1);
    interreqbuilder.setKey(interkey);
    interreqbuilder.addNameEntry(Certificate::EntryCountryName, "GB");
    interreqbuilder.addNameEntry(Certificate::EntryOrganizationName, "Westpoint Intermediate Key");

    CertificateRequest interreq = interreqbuilder.signedRequest(interkey);
    save_request("inter.req", interreq);

    CertificateBuilder interbuilder;
    interbuilder.setRequest(interreq);

    interbuilder.setVersion(3);
    interbuilder.setSerial(RandomGenerator::getPositiveBytes(16));
    interbuilder.setActivationTime(QDateTime::currentDateTimeUtc());
    interbuilder.setExpirationTime(QDateTime::currentDateTimeUtc().addYears(10));
    interbuilder.copyRequestExtensions(interreq);
    interbuilder.setBasicConstraints(true);
    interbuilder.setKeyUsage(CertificateBuilder::UsageCrlSign|CertificateBuilder::UsageKeyCertSign);
    interbuilder.addSubjectKeyIdentifier();
    interbuilder.addAuthorityKeyIdentifier(cacert);

    QSslCertificate intercert = interbuilder.signedCertificate(cacert, cakey);
    save_certificate("inter.crt", intercert);

    //
    // Create the leaf
    //
    QSslKey leafkey = KeyBuilder::generate(QSsl::Rsa, KeyBuilder::StrengthNormal);
    save_key("leaf.key", leafkey);

    CertificateRequestBuilder leafreqbuilder;
    leafreqbuilder.setVersion(1);
    leafreqbuilder.setKey(leafkey);
    leafreqbuilder.addNameEntry(Certificate::EntryCountryName, "GB");
    leafreqbuilder.addNameEntry(Certificate::EntryOrganizationName, "Westpoint");
    leafreqbuilder.addNameEntry(Certificate::EntryCommonName, "127.0.0.1");
    leafreqbuilder.addSubjectAlternativeNameEntry(QSsl::DnsEntry, "127.0.0.1");

    CertificateRequest leafreq = leafreqbuilder.signedRequest(leafkey);
    save_request("leaf.req", leafreq);

    CertificateBuilder leafbuilder;
    leafbuilder.setRequest(leafreq);

    leafbuilder.setVersion(3);
    leafbuilder.setSerial(RandomGenerator::getPositiveBytes(16));
    leafbuilder.setActivationTime(QDateTime::currentDateTimeUtc());
    leafbuilder.setExpirationTime(QDateTime::currentDateTimeUtc().addYears(10));
    leafbuilder.copyRequestExtensions(leafreq);
    leafbuilder.setBasicConstraints(false);
    leafbuilder.addKeyPurpose(CertificateBuilder::PurposeWebServer);
    leafbuilder.setKeyUsage(CertificateBuilder::UsageKeyAgreement|CertificateBuilder::UsageKeyEncipherment);
    leafbuilder.addSubjectKeyIdentifier();
    leafbuilder.addAuthorityKeyIdentifier(intercert);

    QSslCertificate leafcert = leafbuilder.signedCertificate(intercert, interkey);
    save_certificate("leaf.crt", leafcert);
}
