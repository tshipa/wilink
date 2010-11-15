#include <signal.h>
#include <cstdlib>

#include <QCoreApplication>
#include <QSettings>

#include "QXmppLogger.h"

#include "../plugins/phone/sip.h"

#include "phone.h"

static PhoneTester tester;
static int aborted = 0;

static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    tester.stop();
    aborted = 1;
}

PhoneTester::PhoneTester(QObject *parent)
    : QObject(parent)
{
    QSettings settings("sip.conf", QSettings::IniFormat);

    m_client = new SipClient(this);
    m_client->setDomain(settings.value("domain").toString());
    //client.setDisplayName(settings.value("displayName").toString());
    m_client->setUsername(settings.value("username").toString());
    m_client->setPassword(settings.value("password").toString());
    m_phoneNumber = settings.value("phoneNumber").toString();

    connect(m_client, SIGNAL(connected()), this, SIGNAL(connected()));
    connect(m_client, SIGNAL(disconnected()), qApp, SIGNAL(quit()));
}

void PhoneTester::connected()
{
    qDebug("connected to server");
    const QString recipient = QString("sip:%1@%2").arg(m_phoneNumber, m_client->serverName());
    m_client->call(recipient);
}

void PhoneTester::start()
{
    m_client->connectToServer();
}

void PhoneTester::stop()
{
    m_client->disconnectFromServer();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion("1.0.0");

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    QXmppLogger::getLogger()->setLoggingType(QXmppLogger::StdoutLogging);
    tester.start();
    return app.exec();
}

