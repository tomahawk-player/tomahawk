/******************************************************************************
 *   Copyright (C) 2011 Frank Osterfeld <frank.osterfeld@gmail.com>           *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain.h"
#include "keychain_p.h"

using namespace QKeychain;

Job::Job( const QString& service, QObject *parent )
    : QObject( parent )
    , d ( new Private( service ) ) {
}

Job::~Job() {
    delete d;
}

QString Job::service() const {
    return d->service;
}

QSettings* Job::settings() const {
    return d->settings;
}

void Job::setSettings( QSettings* settings ) {
    d->settings = settings;
}

void Job::start() {
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

bool Job::autoDelete() const {
    return d->autoDelete;
}

void Job::setAutoDelete( bool autoDelete ) {
    d->autoDelete = autoDelete;
}

bool Job::insecureFallback() const {
    return d->insecureFallback;
}

void Job::setInsecureFallback( bool insecureFallback ) {
    d->insecureFallback = insecureFallback;
}

void Job::emitFinished() {
    emit finished( this );
    if ( d->autoDelete )
        deleteLater();
}

void Job::emitFinishedWithError( Error error, const QString& errorString ) {
    d->error = error;
    d->errorString = errorString;
    emitFinished();
}

Error Job::error() const {
    return d->error;
}

QString Job::errorString() const {
    return d->errorString;
}

void Job::setError( Error error ) {
    d->error = error;
}

void Job::setErrorString( const QString& errorString ) {
    d->errorString = errorString;
}

ReadPasswordJob::ReadPasswordJob( const QString& service, QObject* parent )
    : Job( service, parent )
    , d( new Private( this ) )
{}

ReadPasswordJob::~ReadPasswordJob() {
    delete d;
}

QString ReadPasswordJob::textData() const {
    return QString::fromUtf8( d->data );
}

QByteArray ReadPasswordJob::binaryData() const {
    return d->data;
}

QString ReadPasswordJob::key() const {
    return d->key;
}

void ReadPasswordJob::setKey( const QString& key ) {
    d->key = key;
}

void ReadPasswordJob::doStart() {
    d->doStart();
}

WritePasswordJob::WritePasswordJob( const QString& service, QObject* parent )
    : Job( service, parent )
    , d( new Private( this ) ) {
}

WritePasswordJob::~WritePasswordJob() {
    delete d;
}

QString WritePasswordJob::key() const {
    return d->key;
}

void WritePasswordJob::setKey( const QString& key ) {
    d->key = key;
}

void WritePasswordJob::setBinaryData( const QByteArray& data ) {
    d->binaryData = data;
    d->mode = Private::Binary;
}

void WritePasswordJob::setTextData( const QString& data ) {
    d->textData = data;
    d->mode = Private::Text;
}

void WritePasswordJob::doStart() {
    d->doStart();
}

DeletePasswordJob::DeletePasswordJob( const QString& service, QObject* parent )
    : Job( service, parent )
    , d( new Private( this ) ) {
}

DeletePasswordJob::~DeletePasswordJob() {
    delete d;
}

void DeletePasswordJob::doStart() {
    //Internally, to delete a password we just execute a write job with no data set (null byte array).
    //In all current implementations, this deletes the entry so this is sufficient
    WritePasswordJob* job = new WritePasswordJob( service(), this );
    connect( job, SIGNAL(finished(QKeychain::Job*)), d, SLOT(jobFinished(QKeychain::Job*)) );
    job->setKey( d->key );
    job->start();
}

QString DeletePasswordJob::key() const {
    return d->key;
}

void DeletePasswordJob::setKey( const QString& key ) {
    d->key = key;
}

void DeletePasswordJob::Private::jobFinished( Job* job ) {
    q->setError( job->error() );
    q->setErrorString( job->errorString() );
    q->emitFinished();
}
