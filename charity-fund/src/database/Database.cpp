#include "Database.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QTextStream>
#include <QByteArray>

namespace {
QString envOr(const char* var, const QString& fallback) {
    QByteArray v = qgetenv(var);
    return v.isEmpty() ? fallback : QString::fromUtf8(v);
}
} // namespace

Database::Database() : connected_(false) {
    connection_ = QSqlDatabase::addDatabase("QPSQL");
}

Database::~Database() {
    disconnect();
}

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::connect() {
    QString host = envOr("CHARITY_DB_HOST", "localhost");
    int port = envOr("CHARITY_DB_PORT", "5432").toInt();
    QString dbname = envOr("CHARITY_DB_NAME", "charity_fund");
    QString user = envOr("CHARITY_DB_USER", "charity_user");
    QString password = envOr("CHARITY_DB_PASSWORD", "charity_pass");

    connection_.setHostName(host);
    connection_.setPort(port);
    connection_.setDatabaseName(dbname);
    connection_.setUserName(user);
    connection_.setPassword(password);

    if (connection_.open()) {
        connected_ = true;
        lastError_.clear();
        QSqlQuery(connection_).exec("SET client_encoding = 'UTF8'");
        if (!migrate()) {
            connected_ = false;
            connection_.close();
            return false;
        }
        return true;
    } else {
        lastError_ = connection_.lastError().text();
        connected_ = false;
        return false;
    }
}

bool Database::migrate() {
    QFile migrationFile(":/migrations.sql");
    if (!migrationFile.open(QFile::ReadOnly | QFile::Text)) {
        lastError_ = "Не вдалося прочитати вбудований migrations.sql";
        return false;
    }

    QTextStream stream(&migrationFile);
    QString sql = stream.readAll();
    migrationFile.close();

    // PostgreSQL's simple query protocol (which QSqlQuery::exec(QString)
    // uses for a plain, non-prepared call) runs a whole ;-separated script
    // as one round trip and understands $$-quoted function bodies, so this
    // does not need fragile client-side statement splitting.
    QSqlQuery query(connection_);
    if (!query.exec(sql)) {
        lastError_ = query.lastError().text();
        return false;
    }
    return true;
}

void Database::disconnect() {
    if (connection_.isOpen()) {
        connection_.close();
        connected_ = false;
    }
}

bool Database::isConnected() const {
    return connected_ && connection_.isOpen();
}

QSqlDatabase& Database::getConnection() {
    // Intentionally does not throw: if the connection has dropped, callers
    // just run QSqlQuery on a closed QSqlDatabase, which fails exec() and
    // is already handled by every Repository method's error path.
    return connection_;
}
