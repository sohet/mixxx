#pragma once

#include <engine/sidechain/networkoutputstreamworker.h>

#include <QMutex>
#include <QSemaphore>
#include <QSharedPointer>
#include <QThread>
#include <QVector>
#include <QWaitCondition>

#include "control/pollingcontrolproxy.h"
#include "encoder/encoder.h"
#include "encoder/encodercallback.h"
#include "preferences/broadcastprofile.h"
#include "preferences/usersettings.h"
#include "track/track_decl.h"
#include "util/fifo.h"

// Forward declare libshout structures to prevent leaking shout.h definitions
// beyond where they are needed.
typedef struct shout shout_t;
typedef struct _util_dict shout_metadata_t;

class QTextCodec;

class ShoutConnection
        : public QThread, public EncoderCallback, public NetworkOutputStreamWorker {
    Q_OBJECT
  public:
    ShoutConnection(BroadcastProfilePtr profile, UserSettingsPointer pConfig);
    ~ShoutConnection() override;

    // This is called by the Engine implementation for each sample. Encode and
    // send the stream, as well as check for metadata changes.
    void process(const CSAMPLE* pBuffer, const std::size_t bufferSize) override;

    void shutdown() override {
    }

    // Called by the encoder in method 'encodebuffer()' to flush the stream to
    // the server.
    void write(const unsigned char* header, const unsigned char* body,
               int headerLen, int bodyLen) override;
    // gets stream position
    int tell() override;
    // sets stream position
    void seek(int pos) override;
    // gets stream length
    int filelen() override;

    /** connects to server **/
    bool serverConnect();
    bool isConnected();
    void applySettings();

    void outputAvailable() override;
    void setOutputFifo(QSharedPointer<FIFO<CSAMPLE>> pOutputFifo) override;
    QSharedPointer<FIFO<CSAMPLE>> getOutputFifo() override;
    bool threadWaiting() override;
    void run() override;

    BroadcastProfilePtr profile() {
        return m_pProfile;
    }

    void setStatus(int newState) {
        m_pProfile->setConnectionStatus(newState);
    }
    int getStatus() {
        return m_pProfile->connectionStatus();
    }

  signals:
    void broadcastDisconnected();
    void broadcastConnected();

  private:
    bool processConnect();
    bool processDisconnect();

    // Update the libshout struct with info from the current broadcast profile.
    void updateFromPreferences();
    int getActiveTracks();
    // Check if the metadata has changed since the previous check.  We also
    // check when was the last check performed to avoid using too much CPU and
    // as well to avoid changing the metadata during scratches.
    bool metaDataHasChanged();
    // Update broadcast metadata. This does not work for OGG/Vorbis and Icecast,
    // since the actual OGG/Vorbis stream contains the metadata.
    void updateMetaData();
    // Common error dialog creation code for run-time exceptions. Notify user
    // when connected or disconnected and so on
    void errorDialog(const QString& text, const QString& detailedError);
    void infoDialog(const QString& text, const QString& detailedError);

    void serverWrite(unsigned char *header, unsigned char *body,
               int headerLen, int bodyLen);

#ifndef __WINDOWS__
    void ignoreSigpipe();
#endif

    bool writeSingle(const unsigned char *data, size_t len);

    QByteArray encodeString(const QString& string);

    bool waitForRetry();

    void tryReconnect();
    void insertMetaData(const char *name, const char *value);

    QTextCodec* m_pTextCodec;
    TrackPointer m_pMetaData;
    shout_t *m_pShout;
    shout_metadata_t *m_pShoutMetaData;
    int m_iMetaDataLife;
    long m_iShoutStatus;
    long m_iShoutFailures;
    UserSettingsPointer m_pConfig;
    BroadcastProfilePtr m_pProfile;
    EncoderPointer m_encoder;
    PollingControlProxy m_mainSamplerate;
    PollingControlProxy m_broadcastEnabled;
    // static metadata according to prefereneces
    bool m_custom_metadata;
    QString m_customArtist;
    QString m_customTitle;
    QString m_metadataFormat;

    // when static metadata is used, we only need calling shout_set_metedata
    // once
    bool m_firstCall;

    bool m_format_is_mp3;
    bool m_format_is_ov;
    bool m_format_is_opus;
    bool m_format_is_aac;
    bool m_protocol_is_icecast1;
    bool m_protocol_is_icecast2;
    bool m_protocol_is_shoutcast;
    bool m_ogg_dynamic_update;
    QAtomicInt m_threadWaiting;
    QSemaphore m_readSema;
    QSharedPointer<FIFO<CSAMPLE>> m_pOutputFifo;

    QString m_lastErrorStr;
    int m_retryCount;

    double m_reconnectFirstDelay;
    double m_reconnectPeriod;
    bool m_noDelayFirstReconnect;
    bool m_limitReconnects;
    int m_maximumRetries;

    QMutex m_enabledMutex;
    QWaitCondition m_waitEnabled;
};

typedef QSharedPointer<ShoutConnection> ShoutConnectionPtr;
